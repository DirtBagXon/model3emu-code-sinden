#include "SDLIncludes.h"
#include <GL/glew.h>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <memory>
#include <thread>
#include <set>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <system_error>
#include <sstream>
#include "GameLoader.h"
#include "../Pkgs/imgui/imgui.h"
#include "../Pkgs/imgui/imgui_impl_sdl2.h"
#include "../Pkgs/imgui/imgui_impl_opengl3.h"
#include "../Src/Util/NewConfig.h"
#include "Util/ConfigBuilders.h"
#include "../Src/OSD/SDL/SDLInputSystem.h"
#include "../Src/Inputs/Inputs.h"
#include "OSD/ConfigFile.h"
#include "Main.h"
#include "Gui.h"

#ifdef _WIN32
    #include "../Src/OSD/Windows/DirectInputSystem.h"
#endif // _WIN32

/*
Quick description on the GUI stuff
----------------------------------

- Using imgui because I didn't want to suck in 1000 other dependencies.
- GUI will only show up if supermodel is run without command line parameters, so existing loaders etc should be completely
  uneffected by this code.
- All controls are completely dynamic, they are created from the settings themselves. So if new settings are added/removed
  they will automatically show up in the GUI, no modifications are required to this code.
- To make this work I added some constraints to the options. For example now you can add a min/max for settings, or add a
  list of possible values. Previously without constraints the code might just silently fail or fall over if you passed
  invalid values.
- The constraints are variants. A c++ variant is exactly the same as a C union, just type safe. So for our options we can
  store bool, unsigned, int, float and std::string. More types could be added but this is enough for now.

  TODO
  ----

 - Some options are just enums which are raw numbers and not user friendly. Probably best replaced with strings.
 - Resolution stuff could be handled better, ie push a list of possible resolutions. Might need to combine into
   a single string or something.
 - Per game options. Need some code to get a diff of possible configs I think.
*/


// ---------------------------------------------------------------------------------------------------------------
// ROM folder scan:
//
// A game is considered owned if its name is:
//   1. The name of a .zip file in the ROM folder, or
//   2. The first directory component of an entry inside one of those .zip files.
//
// No ROM filenames, CRCs, XML definitions, etc. are used to determine ownership.
//
// Example:
//
//   ROMs/scud.zip
//
//   scud.zip contains:
//       scuddx/file1.bin
//       scuddx/file2.bin
//       scudplus/file1.bin
//
// The owned game names are:
//       scud
//       scuddx
//       scudplus
//
// Nested directories do NOT create additional game names. For example:
//
//       scudx/subdir/file.bin
//
// only contributes "scudx".
// ---------------------------------------------------------------------------------------------------------------

static std::string ToLowerStr(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    return s;
}

// Returns the lower case game names represented by all *.zip files in the given directory.
//
// Each ZIP contributes:
//   - its own filename without the .zip extension
//   - the first directory component of every file stored inside the ZIP
//
// Only the first directory component is used. No file names inside the ZIP
// are themselves considered game names.
static std::set<std::string> ScanRomZips(const std::string& dir,
                                          std::set<std::string>& subfolderGames,
                                          std::map<std::string, std::string>& romPaths)
{
    std::set<std::string> found;
    std::error_code ec;

    for (std::filesystem::directory_iterator it(dir, ec), end; !ec && it != end; it.increment(ec)) {
        std::error_code ec2;

        if (!it->is_regular_file(ec2)) {
            continue;
        }

        if (ToLowerStr(it->path().extension().string()) != ".zip") {
            continue;
        }

        const std::string zipFileName = it->path().filename().string();
        const std::string zipGameName = ToLowerStr(it->path().stem().string());
        const std::string zipPath = it->path().string();

        // Store the filename; GetRomPath joins it to the configured ROM directory.
        // Keep zipPath here for opening the archive during this scan.
        found.insert(zipGameName);
        romPaths[zipGameName] = zipFileName;

        // Inspect the contents of the ZIP for additional game names.
        //
        // GameLoader already uses minizip for reading ZIP archives, so use
        // the same API here. This is only an enumeration pass; no ROM data
        // is opened or extracted.
        unzFile zf = unzOpen(zipPath.c_str());
        if (zf == nullptr) {
            continue;
        }

        for (int zipErr = unzGoToFirstFile(zf);
             zipErr == UNZ_OK;
             zipErr = unzGoToNextFile(zf))
        {
            char filename[1024] = {};

            if (unzGetCurrentFileInfo(
                    zf,
                    nullptr,
                    filename,
                    sizeof(filename),
                    nullptr,
                    0,
                    nullptr,
                    0) != UNZ_OK)
            {
                continue;
            }

            std::string entryName(filename);

            // ZIP paths conventionally use '/', but accept '\\' as well.
            const size_t separator = entryName.find_first_of("/\\");

            // An entry without a directory is not a game name.
            if (separator == std::string::npos || separator == 0) {
                continue;
            }

            // Only the first directory component is a game name.
            //
            // For:
            //     game/subdir/rom.bin
            //
            // only "game" is added.
            const std::string gameName = ToLowerStr(entryName.substr(0, separator));

            if (!gameName.empty()) {
                found.insert(gameName);
                subfolderGames.insert(gameName);
                romPaths[gameName] = zipFileName;
            }
        }

        unzClose(zf);
    }

    return found;
}

static void WriteGameNode(Util::Config::Node& baseNode, const Util::Config::Node& diffNode, Util::Config::Node& writeNode, const std::string& group)
{
    for (const auto& n : baseNode) {

        if (n.IsLeaf() && n.Exists()) {

            auto& key = n.Key();
            auto  val = n.GetValue();

            if (val) {

                auto vRange = val->GetValueRange();

                if (vRange->GetGroup() != group) {
                    continue;
                }

                auto s1 = baseNode[key].ValueAs<std::string>();
                auto s2 = diffNode[key].ValueAs<std::string>();

                if (s1 != s2) {
                    writeNode[key] = s2;
                }
            }
        }
    }
}

static std::string NodeToString(Util::Config::Node& config)
{
    std::string s;

    for (const auto& n : config) {

        if (n.IsLeaf() && n.Exists()) {

            auto& key = n.Key();

            auto val = config[key].ValueAs<std::string>();

            s += key;
            s += " ";
            s += val;
            s += "\n";
        }
    }

    return s;
}

static void UpdateTempValues(Util::Config::Node& config, const std::string& group, bool init)
{
    for (auto it = config.begin(); it != config.end(); ++it)
    {
        if (it->IsLeaf() && it->Exists()) {

            const auto& key = it->Key();
            auto val = it->GetValue();

            if (val) {

                auto vRange = val->GetValueRange();

                if (vRange) {

                    if (vRange->GetGroup() == group) {

                        auto index = vRange->GetIndex();

                        switch (index) {
                        case 0:             // bool
                        {
                            if (init) { vRange->tempValue = config[key].ValueAs<bool>(); }
                            else { config.Set(key, std::get<bool>(vRange->tempValue)); }
                            break;
                        }
                        case 1:             // unsigned
                        {
                            if (init) { vRange->tempValue = config[key].ValueAs<unsigned>(); }
                            else { config.Set(key, std::get<unsigned>(vRange->tempValue)); }
                            break;
                        }
                        case 2:             // int
                        {
                            if (init) { vRange->tempValue = config[key].ValueAs<int>(); }
                            else { config.Set(key, std::get<int>(vRange->tempValue)); }
                            break;
                        }
                        case 3:             // float
                        {
                            if (init) { vRange->tempValue = config[key].ValueAs<float>(); }
                            else { config.Set(key, std::get<float>(vRange->tempValue)); }
                            break;
                        }
                        case 4:             // std::string
                        {
                            if (init) { vRange->tempValue = config[key].ValueAs<std::string>(); }
                            else { config.Set(key, std::get<std::string>(vRange->tempValue)); }
                            break;
                        }
                        }

                        it->GetValue()->SetValueRange(vRange);      // setting a new key could erase value range so make sure to re-add it
                    }
                }
            }  
        }
    }
}

template <typename T>
static std::string ValueToString(const T& value)
{
    std::ostringstream ss;
    ss << value;
    return ss.str();
}

template <typename T>
static bool ValuesMatch(const T& a, const T& b)
{
    return a == b;
}

// Floats are written to the INI file with 6 significant digits, so compare with a tolerance
static bool ValuesMatch(float a, float b)
{
    return std::fabs(a - b) <= 1e-5f * std::fabs(b);
}

// Combo box for a setting restricted to a list of values, showing the entries' labels if it has
// them. A current value that is not in the list (e.g. set by hand in the INI file or on the
// command line) is shown as an extra "Custom" entry and kept unless another entry is picked.
// Lets ImGui::InputText edit a std::string directly
static int InputTextResizeCallback(ImGuiInputTextCallbackData* data)
{
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
        auto str = static_cast<std::string*>(data->UserData);
        str->resize(data->BufTextLen);
        data->Buf = str->data();
    }
    return 0;
}

template <typename T>
static void ValueCombo(const std::string& key, Util::ValueRange& vRange, const T& current)
{
    auto& list = vRange.GetList();
    auto& labels = vRange.GetLabels();
    bool useLabels = labels.size() == list.size();

    std::vector<std::string> items;
    int selectedIndex = -1;

    for (size_t i = 0; i < list.size(); i++) {
        const auto& value = std::get<T>(list[i]);
        items.emplace_back(useLabels ? labels[i] : ValueToString(value));
        if (ValuesMatch(value, current)) {
            selectedIndex = (int)i;
        }
    }

    if (selectedIndex < 0) {
        items.emplace_back("Custom (" + ValueToString(current) + ')');
        selectedIndex = (int)list.size();
    }

    std::vector<const char*> itemPtrs;
    for (auto& s : items) {
        itemPtrs.emplace_back(s.c_str());
    }

    if (ImGui::Combo(key.c_str(), &selectedIndex, itemPtrs.data(), (int)itemPtrs.size()) && selectedIndex < (int)list.size()) {
        vRange.tempValue = list[selectedIndex];
    }
}

static void CreateControls(Util::Config::Node& config, const std::string& group)
{
    for (auto it = config.begin(); it != config.end(); ++it)
    {
        if (it->IsLeaf() && it->Exists()) {

            const auto& key = it->Key();
            auto val = it->GetValue();

            if (val) {

                auto vRange = val->GetValueRange();

                if (vRange) {

                    if (vRange->GetGroup() == group) {

                        auto index = vRange->GetIndex();
                        auto& list = vRange->GetList();

                        auto ProcessScalar = [&](auto valuePtr, ImGuiDataType type)
                        {
                            using T = std::decay_t<decltype(*valuePtr)>;
                            auto min_ = std::get<T>(vRange->GetMin());
                            auto max_ = std::get<T>(vRange->GetMax());

                            // optional display text: a unit, or a name for the minimum value
                            const char* format = nullptr;
                            if (!vRange->GetSliderMinLabel().empty() && *valuePtr == min_) {
                                format = vRange->GetSliderMinLabel().c_str();
                            }
                            else if (!vRange->GetSliderFormat().empty()) {
                                format = vRange->GetSliderFormat().c_str();
                            }

                            ImGui::SliderScalar(key.c_str(), type, valuePtr, &min_, &max_, format);
                        };
                        
                        auto ProcessControls = [&](auto valuePtr, ImGuiDataType type)
                        {
                            if (vRange->HasMinMax()) {
                                ProcessScalar(valuePtr, type);
                            }
                            else if (list.size()) {
                                ValueCombo(key, *vRange, *valuePtr);
                            }
                            else {
                                ImGui::InputScalar(key.c_str(), type, valuePtr);
                            }
                        };


                        switch (index) {
                        case 0:             // bool
                        {
                            auto p = std::get_if<bool>(&vRange->tempValue);
                            ImGui::Checkbox(key.c_str(), p);
                            break;
                        }
                        case 1:             // unsigned
                        {
                            auto p = std::get_if<unsigned>(&vRange->tempValue);
                            ProcessControls(p, ImGuiDataType_U32);
                            break;
                        }
                        case 2:             // int
                        {
                            auto p = std::get_if<int>(&vRange->tempValue);
                            ProcessControls(p, ImGuiDataType_S32);
                            break;
                        }
                        case 3:             // float
                        {
                            auto p = std::get_if<float>(&vRange->tempValue);
                            ProcessControls(p, ImGuiDataType_Float);
                            break;
                        }
                        case 4:             // std::string
                        {
                            auto& option = std::get<std::string>(vRange->tempValue);

                            if (list.size()) {
                                ValueCombo(key, *vRange, option);
                            }
                            else {
                                // edit the string directly, it grows as needed
                                ImGui::InputText(key.c_str(), option.data(), option.capacity() + 1, ImGuiInputTextFlags_CallbackResize, InputTextResizeCallback, &option);
                            }

                            break;
                        }
                        }
                    }
                }
            }
        }
    }
}

static void SetDefaultKeyVal(std::shared_ptr<CInput> input)
{
    std::string key = std::string("Input") + input->id;

    auto defaultConfig = DefaultConfig();

    auto mapping = defaultConfig[key].ValueAs<std::string>();

    // update input with value from our default config
    input->SetMapping(mapping.c_str());
}

static std::vector<std::string> SplitByComma(const std::string& input) 
{
    std::vector<std::string> result;
    size_t start = 0;
    size_t end;

    while ((end = input.find(',', start)) != std::string::npos) {
        result.emplace_back(input.substr(start, end - start));
        start = end + 1;
    }

    // Add the last segment (or the whole string if no commas)
    result.emplace_back(input.substr(start));
    return result;
}

struct KeyBindState
{
    std::shared_ptr<CInput> input;
    bool waitingForInput = false;
    int processKeyCount = 0;            // we need to let the gui draw a frame or two before blocking for key input

    void Reset()
    {
        input = nullptr;
        waitingForInput = false;
        processKeyCount = 0;
    }
};

static void BindKeys(Util::Config::Node& config, KeyBindState& kb, bool openPopup)
{
    bool finish = false;
    bool appendPressed = false;

    if (openPopup) {
        ImGui::OpenPopup("Key Binding");
    }

    if (ImGui::BeginPopupModal("Key Binding", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {

        if (ImGui::BeginTable("ShortcutTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {

            ImGui::TableSetupColumn("Action");
            ImGui::TableSetupColumn("Keys");
            ImGui::TableHeadersRow();

            auto keyList = SplitByComma(kb.input->GetMapping());

            for (auto& k : keyList) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%s", kb.input->label);
                ImGui::TableNextColumn();
                ImGui::Text("%s", k.c_str());
            }

            ImGui::EndTable();
        }

        ImGui::Spacing(); // Adds default vertical spacing

        if (kb.waitingForInput) {

            ImVec4 color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // bright yellow
            color.w = 0.7f + 0.3f * 0.1f; // modulate alpha

            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::Text("Waiting for key input. Press esc to cancel.");
            ImGui::PopStyleColor();

            kb.processKeyCount++;
        }
        else {
            ImGui::NewLine();
        }
        ImGui::Spacing(); // Adds default vertical spacing


        if (kb.waitingForInput) {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("Set", ImVec2(120, 0))) {
            kb.input->ClearMapping();
            appendPressed = true;
        }

        if (ImGui::Button("Append", ImVec2(120, 0))) {
            appendPressed = true;
        }

        if (ImGui::Button("Clear", ImVec2(120, 0))) {
            kb.input->ClearMapping();
            kb.input->StoreToConfig(&config);
        }

        if (ImGui::Button("Default", ImVec2(120, 0))) {
            SetDefaultKeyVal(kb.input);
            kb.input->StoreToConfig(&config);
        }

        if (ImGui::Button("Finish", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
            finish = true;
        }

        if (kb.waitingForInput) {
            ImGui::EndDisabled();
        }

        ImGui::EndPopup();

        if (kb.processKeyCount > 2) {           // kind of cludge logic. We need to draw at least once to update the GUI, configure is a blocking function so the GUI will basically freeze until we press a button. Time out might make sense

            auto system = kb.input->GetInputSystem();

            system->UngrabMouse();
            kb.input->Configure(true);
            system->GrabMouse();

            kb.input->StoreToConfig(&config);
            kb.processKeyCount = 0;
            kb.waitingForInput = false;
        }

        if (appendPressed) {
            kb.waitingForInput = true;
        }

        if (finish) {
            kb.Reset();
        }
    }
}

static void AddKeys(Util::Config::Node& config, KeyBindState& kb, const std::vector<std::shared_ptr<CInput>>& keyInputs)
{
    bool openPopup = false;

    for (auto& k : keyInputs) {
        auto mapping    = k->GetMapping();
        auto group      = k->GetInputGroup();
        auto label      = k->label;

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("%s", group);
        ImGui::TableSetColumnIndex(1);
        if (ImGui::Selectable(label, false, ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_SpanAllColumns)) {
            if (ImGui::IsMouseDoubleClicked(0)) {
                kb.input = k;
                openPopup = true;
            }
        }
        ImGui::TableSetColumnIndex(2);
        ImGui::Text("%s", mapping);
    }

    BindKeys(config, kb, openPopup);
}

static void DrawButtonOptions(Util::Config::Node& config, int& selectedGameIndex, bool& exit, bool& saveSettings, std::shared_ptr<CInputs>& inputs, bool closeRequested, bool& exitPromptOpen)
{
    if (ImGui::Button("Load game")) {

        if (selectedGameIndex < 0) {
            ImGui::OpenPopup("Load game");
        }
        else {
            exit = true;
        }
    }

    if (ImGui::BeginPopupModal("Load game", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {

        ImGui::Text("No game selected");
        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2(120, 0))) { 
            ImGui::CloseCurrentPopup(); 
        }

        ImGui::EndPopup();
    }

    ImGui::SameLine();

    if (ImGui::Button("Load Defaults")) {
        ImGui::OpenPopup("Confirm Load");
    }

    if (ImGui::BeginPopupModal("Confirm Load", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {

        ImGui::Text("Are you sure you want to load defaults?");
        ImGui::Separator();

        if (ImGui::Button("Yes", ImVec2(120, 0))) { 
            config = DefaultConfig();
            inputs = nullptr; // rebuilt from the defaults when Key bindings is next shown
            ImGui::CloseCurrentPopup(); 
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) { 
            ImGui::CloseCurrentPopup(); 
        }

        ImGui::EndPopup();
    }

    ImGui::SameLine();

    if (ImGui::Button("Exit") || closeRequested) {
        ImGui::OpenPopup("Save On Exit");
    }

    exitPromptOpen = ImGui::BeginPopupModal("Save On Exit", NULL, ImGuiWindowFlags_AlwaysAutoResize);
    if (exitPromptOpen) {

        ImGui::Text("Save settings upon exit?");
        ImGui::Separator();

        if (ImGui::Button("Yes", ImVec2(120, 0))) {
            selectedGameIndex = -1;
            saveSettings = true;
            exit = true;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("No", ImVec2(120, 0))) {
            selectedGameIndex = -1;
            saveSettings = false;
            exit = true;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    {
        const bool disabled = true;

        if (disabled) {
            ImGui::BeginDisabled(); // Disables interaction
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f); // Fade visuals
        }

        static bool perGameSettings = false;
        bool perGamesSettingsCopy = perGameSettings;
        ImGui::Checkbox("Per game settings", &perGamesSettingsCopy);



        if (disabled) {
            ImGui::PopStyleVar();
            ImGui::EndDisabled();
        }
    }
}

static std::shared_ptr<CInputs> GetInputSystem(Util::Config::Node& config, SDL_Window* window)
{
    std::string selectedInputSystem = config["InputSystem"].ValueAs<std::string>();

    std::shared_ptr<CInputSystem> inputSystem;

    if (selectedInputSystem == "sdl")               { inputSystem = std::shared_ptr<CInputSystem>(new CSDLInputSystem(config, false));}
    else if (selectedInputSystem == "sdlgamepad")   { inputSystem = std::shared_ptr<CInputSystem>(new CSDLInputSystem(config, true)); }

#ifdef SUPERMODEL_WIN32
    else if (selectedInputSystem == "dinput")       { inputSystem = std::shared_ptr<CInputSystem>(new CDirectInputSystem(config, window, false, false));}
    else if (selectedInputSystem == "xinput")       { inputSystem = std::shared_ptr<CInputSystem>(new CDirectInputSystem(config, window, false, true)); }
    else if (selectedInputSystem == "rawinput")     { inputSystem = std::shared_ptr<CInputSystem>(new CDirectInputSystem(config, window, true, false)); }
#endif // SUPERMODEL_WIN32

    // initialise 
    if (inputSystem) {
        auto inputs = std::shared_ptr<CInputs>(new CInputs(inputSystem));
        inputs->Initialize();
        inputs->LoadFromConfig(config); 

        int x, y, w, h;
        SDL_GetWindowPosition(window, &x, &y);
        SDL_GL_GetDrawableSize(window, &w, &h);

        inputSystem->SetDisplayGeom(x, y, w, h);
        return inputs;
    }

    return nullptr;
}

static void DrawInputSystemError(const Util::Config::Node& config)
{
    ImGui::TextWrapped("Input system \"%s\" is not available on this platform. Choose another one under Core > InputSystem.",
        config["InputSystem"].ValueAs<std::string>().c_str());
}

static Game GetGame(const std::vector<const Game*>& visibleGames, int selectedGameIndex)
{
    if (selectedGameIndex >= 0 && selectedGameIndex < (int)visibleGames.size()) {
        return *visibleGames[selectedGameIndex];
    }

    return Game{};
}

static void GUI(const ImGuiIO& io, Util::Config::Node& config, const std::vector<const Game*>& visibleGames, int& selectedGameIndex, bool& exit, bool& saveSettings, SDL_Window* window, std::shared_ptr<CInputs>& inputs, KeyBindState& kb, bool closeRequested, bool& exitPromptOpen)
{
    ImVec4 clear_color = ImVec4(0.0f, 0.5f, 192/255.f, 1.00f);

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

    ImGui::Begin("Custom Window", nullptr, ImGuiWindowFlags_NoTitleBar); // Explicitly set a window name

    ImGui::BeginChild("TableRegion", ImVec2(0.0f, 200.0f), true, ImGuiWindowFlags_HorizontalScrollbar);

    if (ImGui::BeginTable("Games", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit))
    {
        ImGui::TableSetupColumn("Title");
        ImGui::TableSetupColumn("Rom Name");
        ImGui::TableSetupColumn("Version");
        ImGui::TableSetupColumn("Year");
        ImGui::TableSetupColumn("Stepping");

        ImGui::TableHeadersRow();

        int row = 0;
        for (const Game* game : visibleGames) {
            const Game& g = *game;

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", g.title.c_str());
            ImGui::TableSetColumnIndex(1);
            if (ImGui::Selectable(g.name.c_str(), selectedGameIndex == row, ImGuiSelectableFlags_SpanAllColumns)) {
                selectedGameIndex = row;
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                exit = true;
            }

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", g.version.c_str());
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%d", g.year);
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%s", g.stepping.c_str());

            row++;
        }

        ImGui::EndTable();
    }

    ImGui::EndChild();

    // draw button options
    DrawButtonOptions(config, selectedGameIndex, exit, saveSettings, inputs, closeRequested, exitPromptOpen);

    // create a space
    ImGui::Dummy(ImVec2(0.0f, 20.0f));

    // draw the tabbed options
    if (ImGui::BeginTabBar("MyTabBar", ImGuiTabBarFlags_FittingPolicyResizeDown)) {
        if (ImGui::BeginTabItem("Core")) {
            UpdateTempValues(config, "Core", true);
            CreateControls(config, "Core");
            UpdateTempValues(config, "Core", false);
            ImGui::EndTabItem();
            inputs = nullptr;
        }
        if (ImGui::BeginTabItem("Video")) {
            UpdateTempValues(config, "Video", true);
            CreateControls(config, "Video");
            UpdateTempValues(config, "Video", false);
            ImGui::EndTabItem();
            inputs = nullptr;
        }
        if (ImGui::BeginTabItem("Legacy3D")) {
            UpdateTempValues(config, "Legacy3D", true);
            CreateControls(config, "Legacy3D");
            UpdateTempValues(config, "Legacy3D", false);
            ImGui::EndTabItem();
            inputs = nullptr;
        }
        if (ImGui::BeginTabItem("Audio")) {
            UpdateTempValues(config, "Sound", true);
            CreateControls(config, "Sound");
            UpdateTempValues(config, "Sound", false);
            ImGui::EndTabItem();
            inputs = nullptr;
        }
        if (ImGui::BeginTabItem("Networking")) {
            UpdateTempValues(config, "Network", true);
            CreateControls(config, "Network");
            UpdateTempValues(config, "Network", false);
            ImGui::EndTabItem();
            inputs = nullptr;
        }
        if (ImGui::BeginTabItem("Misc")) {
            UpdateTempValues(config, "Misc", true);
            CreateControls(config, "Misc");
            UpdateTempValues(config, "Misc", false);
            ImGui::EndTabItem();
            inputs = nullptr;
        }
        if (ImGui::BeginTabItem("ForceFeedback")) {
            UpdateTempValues(config, "ForceFeedback", true);
            CreateControls(config, "ForceFeedback");
            UpdateTempValues(config, "ForceFeedback", false);
            ImGui::EndTabItem();
            inputs = nullptr;
        }
        if (ImGui::BeginTabItem("Sensitivity")) {
            UpdateTempValues(config, "Sensitivity", true);
            CreateControls(config, "Sensitivity");
            UpdateTempValues(config, "Sensitivity", false);
            if (ImGui::Button("Joystick calibration")) {
                if (inputs == nullptr) {
                    if (inputs == nullptr) {
                        inputs = GetInputSystem(config, window);
                    }
                }
                inputs->CalibrateJoysticks();
                inputs->StoreToConfig(&config);
            }
            ImGui::EndTabItem();
            inputs = nullptr;
        }
        if (ImGui::BeginTabItem("Key bindings")) {

            if (inputs == nullptr) {
                inputs = GetInputSystem(config, window);
            }

            if (inputs == nullptr) {
                DrawInputSystemError(config);
            }
            else if (ImGui::BeginTable("KeyTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {

                auto inputList = inputs->GetGameInputs(GetGame(visibleGames, selectedGameIndex));

                ImGui::TableSetupColumn("Group");
                ImGui::TableSetupColumn("Action");
                ImGui::TableSetupColumn("Keys");
                ImGui::TableHeadersRow();

                AddKeys(config, kb, inputList);

                ImGui::EndTable();
            }
            
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End(); // Close the window


    // Rendering
    ImGui::Render();
    
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

static std::string GetRomPath(int selectedGame,
                               const std::vector<const Game*>& visibleGames,
                               const std::map<std::string, std::string>& romPaths,
                               const std::string& romDirectory)
{
    if (selectedGame >= 0 && selectedGame < (int)visibleGames.size()) {
        const std::string gameName = ToLowerStr(visibleGames[selectedGame]->name);
        auto it = romPaths.find(gameName);
        if (it != romPaths.end()) {
            return (std::filesystem::path(romDirectory) / it->second).string();
        }
    }

    return {};  // no game
}

static float GetDPIScale(SDL_Window* window)
{
    int displayIndex = SDL_GetWindowDisplayIndex(window);
    float ddpi = 96.0f; // Default fallback
    if (SDL_GetDisplayDPI(displayIndex, &ddpi, nullptr, nullptr) != 0) {
        SDL_Log("Failed to get DPI: %s", SDL_GetError());
    }

    return ddpi / 96.0f;
}

std::vector<std::string> RunGUI(const std::string& configPath, const Util::Config::Node& fileConfig)
{
    // Our own copy of the complete config file. The GUI only ever edits the global
    // section but the game-specific sections are carried along so that they survive
    // being written back out.
    Util::Config::Node fullConfig = fileConfig;

    // The global section, merged over the defaults, is what the GUI operates on. The
    // merge is also what attaches the value ranges the controls are built from, so it
    // has to be done here: copying a node does not preserve them.
    Util::Config::Node config("Global");
    Util::Config::MergeINISections(&config, DefaultConfig(), fullConfig);

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! Error: " << SDL_GetError() << std::endl;
        return {};
    }

    // Set OpenGL attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_INPUT_FOCUS);

    // Create SDL window
    SDL_Window* window = SDL_CreateWindow("SuperSetup", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 950, 600, window_flags);
    if (!window) {
        std::cerr << "Window could not be created! Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return {};
    }

    // Create OpenGL context
    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        std::cerr << "OpenGL context could not be created! Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return {};
    }

    SDL_GL_MakeCurrent(window, glContext);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    io.Fonts->AddFontDefaultVector();

    ImGui::GetIO().IniFilename = nullptr;                      // we don't need to save window positions between runs

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();
    //ImGui::StyleColorsClassic();

    ImGuiStyle& style = ImGui::GetStyle();
    float scale = GetDPIScale(window);
    //style.ScaleAllSizes(scale);

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init("#version 410");

    std::string xmlFile = config["GameXMLFile"].ValueAs<std::string>();
    GameLoader loader(xmlFile);
    auto& games = loader.GetGames();

    // Scan the ROM folder once at startup. ownedZips contains only names that
    // come from ZIP filenames or first-level directories inside those ZIPs.
    // romPaths maps each owned game to the ZIP that contains it.
    std::set<std::string> subfolderGames;
    std::map<std::string, std::string> romPaths;
    const std::string romDirectory = config["ROMDirectory"].ValueAs<std::string>();
    const std::set<std::string> ownedZips = ScanRomZips(romDirectory, subfolderGames, romPaths);

    // Build the actual GUI list from owned games only. This means selection
    // indices always refer to the visible list, not the complete XML game map.
    std::vector<const Game*> visibleGames;
    for (const auto& g : games) {
        if (ownedZips.count(ToLowerStr(g.second.name)) > 0) {
            visibleGames.push_back(&g.second);
        }
    }

    int selectedGame = -1;  // -1 means no selection
    std::vector<std::string> romFiles;
    std::string path;

    // Main loop
    std::shared_ptr<CInputs> inputs;
    bool saveSettings = true;
    bool running = true;
    KeyBindState kb{};
    bool exitPromptOpen = false;
    bool exit = false;
    SDL_Event event{};

    while (running) {

        // Closing the window asks to save like Exit does; closing it again while asked exits without saving
        bool closeRequested = false;

        while (SDL_PollEvent(&event)) {

            ImGui_ImplSDL2_ProcessEvent(&event);

            if (event.type == SDL_QUIT) {
                if (exitPromptOpen) {
                    goto exitNoSave;
                }
                closeRequested = true;
            }
        }

        GUI(io, config, visibleGames, selectedGame, exit, saveSettings, window, inputs, kb, closeRequested, exitPromptOpen);

        std::this_thread::sleep_for(std::chrono::milliseconds(5));

        SDL_GL_SwapWindow(window);

        if (exit) {
            break;
        }
    }

    path = GetRomPath(selectedGame, visibleGames, romPaths, romDirectory);
    if (!path.empty()) {
        romFiles.emplace_back(path);

        // A game found as a first-level directory inside a ZIP is selected
        // from that ZIP using Supermodel's -game=<name> argument.
        if (selectedGame >= 0 && selectedGame < (int)visibleGames.size()) {
            const std::string gameName = ToLowerStr(visibleGames[selectedGame]->name);
            if (subfolderGames.count(gameName) > 0) {
                romFiles.emplace_back("-game=" + visibleGames[selectedGame]->name);
            }
        }
    }
    
    if (saveSettings) {
        // Fold the edited global settings back into the complete config so that the
        // game-specific sections are preserved by the write
        Util::Config::CopyINISection(&fullConfig, config);
        Util::Config::WriteINIFile(configPath, fullConfig, TimestampedConfigFileComment("Updated from GUI."));
    }

exitNoSave:

    // Release the input systems while SDL is still up
    kb.Reset();
    inputs = nullptr;

    // Cleanup resources
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return romFiles;
}
