/**
 ** Supermodel
 ** A Sega Model 3 Arcade Emulator.
 ** Copyright 2011 Bart Trzynadlowski, Nik Henson
 **
 ** This file is part of Supermodel.
 **
 ** Supermodel is free software: you can redistribute it and/or modify it under
 ** the terms of the GNU General Public License as published by the Free
 ** Software Foundation, either version 3 of the License, or (at your option)
 ** any later version.
 **
 ** Supermodel is distributed in the hope that it will be useful, but WITHOUT
 ** ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 ** FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 ** more details.
 **
 ** You should have received a copy of the GNU General Public License along
 ** with Supermodel.  If not, see <http://www.gnu.org/licenses/>.
 **/

/*
 * SDLInputSystem.cpp
 *
 * Implementation of SDL input system.
 *
 * TODO:
 * -----
 * - Implement multiple keyboard and mouse support.
 */

#include "SDLInputSystem.h"

#include "Supermodel.h"
#include "Inputs/Input.h"
#include "Inputs/Manymouse.h"

#include <iostream>
#include <vector>
using namespace std;

static int available_mice = 1;
static ManyMouseEvent mm_event;

SDLKeyMapStruct CSDLInputSystem::s_keyMap[] =
{
  // General keys
  { "BACKSPACE",      SDL_SCANCODE_BACKSPACE },
  { "TAB",            SDL_SCANCODE_TAB },
  { "CLEAR",          SDL_SCANCODE_CLEAR },
  { "RETURN",         SDL_SCANCODE_RETURN },
  { "PAUSE",          SDL_SCANCODE_PAUSE },
  { "ESCAPE",         SDL_SCANCODE_ESCAPE },
  { "SPACE",          SDL_SCANCODE_SPACE },
  { "QUOTE",          SDL_SCANCODE_APOSTROPHE },
  { "LEFTPAREN",      SDL_SCANCODE_KP_LEFTPAREN },
  { "RIGHTPAREN",     SDL_SCANCODE_KP_RIGHTPAREN },
  { "COMMA",          SDL_SCANCODE_COMMA },
  { "MINUS",          SDL_SCANCODE_MINUS },
  { "PERIOD",         SDL_SCANCODE_PERIOD },
  { "SLASH",          SDL_SCANCODE_SLASH },
  { "0",              SDL_SCANCODE_0 },
  { "1",              SDL_SCANCODE_1 },
  { "2",              SDL_SCANCODE_2 },
  { "3",              SDL_SCANCODE_3 },
  { "4",              SDL_SCANCODE_4 },
  { "5",              SDL_SCANCODE_5 },
  { "6",              SDL_SCANCODE_6 },
  { "7",              SDL_SCANCODE_7 },
  { "8",              SDL_SCANCODE_8 },
  { "9",              SDL_SCANCODE_9 },
  { "SEMICOLON",      SDL_SCANCODE_SEMICOLON },
  { "EQUALS",         SDL_SCANCODE_EQUALS },
  { "LEFTBRACKET",    SDL_SCANCODE_LEFTBRACKET },
  { "BACKSLASH",      SDL_SCANCODE_BACKSLASH },
  { "RIGHTBRACKET",   SDL_SCANCODE_RIGHTBRACKET },
  { "BACKQUOTE",      SDL_SCANCODE_GRAVE },
  { "A",              SDL_SCANCODE_A },
  { "B",              SDL_SCANCODE_B },
  { "C",              SDL_SCANCODE_C },
  { "D",              SDL_SCANCODE_D },
  { "E",              SDL_SCANCODE_E },
  { "F",              SDL_SCANCODE_F },
  { "G",              SDL_SCANCODE_G },
  { "H",              SDL_SCANCODE_H },
  { "I",              SDL_SCANCODE_I },
  { "J",              SDL_SCANCODE_J },
  { "K",              SDL_SCANCODE_K },
  { "L",              SDL_SCANCODE_L },
  { "M",              SDL_SCANCODE_M },
  { "N",              SDL_SCANCODE_N },
  { "O",              SDL_SCANCODE_O },
  { "P",              SDL_SCANCODE_P },
  { "Q",              SDL_SCANCODE_Q },
  { "R",              SDL_SCANCODE_R },
  { "S",              SDL_SCANCODE_S },
  { "T",              SDL_SCANCODE_T },
  { "U",              SDL_SCANCODE_U },
  { "V",              SDL_SCANCODE_V },
  { "W",              SDL_SCANCODE_W },
  { "X",              SDL_SCANCODE_X },
  { "Y",              SDL_SCANCODE_Y },
  { "Z",              SDL_SCANCODE_Z },
  { "DEL",            SDL_SCANCODE_DELETE },

  // Keypad
  { "KEYPAD0",        SDL_SCANCODE_KP_0 },
  { "KEYPAD1",        SDL_SCANCODE_KP_1 },
  { "KEYPAD2",        SDL_SCANCODE_KP_2 },
  { "KEYPAD3",        SDL_SCANCODE_KP_3 },
  { "KEYPAD4",        SDL_SCANCODE_KP_4 },
  { "KEYPAD5",        SDL_SCANCODE_KP_5 },
  { "KEYPAD6",        SDL_SCANCODE_KP_6 },
  { "KEYPAD7",        SDL_SCANCODE_KP_7 },
  { "KEYPAD8",        SDL_SCANCODE_KP_8 },
  { "KEYPAD9",        SDL_SCANCODE_KP_9 },
  { "KEYPADPERIOD",   SDL_SCANCODE_KP_PERIOD },
  { "KEYPADDIVIDE",   SDL_SCANCODE_KP_DIVIDE },
  { "KEYPADMULTIPLY", SDL_SCANCODE_KP_MULTIPLY },
  { "KEYPADMINUS",    SDL_SCANCODE_KP_MINUS },
  { "KEYPADPLUS",     SDL_SCANCODE_KP_PLUS },
  { "KEYPADENTER",    SDL_SCANCODE_KP_ENTER },
  { "KEYPADEQUALS",   SDL_SCANCODE_KP_EQUALS },

  // Arrows + Home/End Pad
  { "UP",             SDL_SCANCODE_UP },
  { "DOWN",           SDL_SCANCODE_DOWN },
  { "RIGHT",          SDL_SCANCODE_RIGHT },
  { "LEFT",           SDL_SCANCODE_LEFT },
  { "INSERT",         SDL_SCANCODE_INSERT },
  { "HOME",           SDL_SCANCODE_HOME },
  { "END",            SDL_SCANCODE_END },
  { "PGUP",           SDL_SCANCODE_PAGEUP },
  { "PGDN",           SDL_SCANCODE_PAGEDOWN },

  // Function Key
  { "F1",             SDL_SCANCODE_F1 },
  { "F2",             SDL_SCANCODE_F2 },
  { "F3",             SDL_SCANCODE_F3 },
  { "F4",             SDL_SCANCODE_F4 },
  { "F5",             SDL_SCANCODE_F5 },
  { "F6",             SDL_SCANCODE_F6 },
  { "F7",             SDL_SCANCODE_F7 },
  { "F8",             SDL_SCANCODE_F8 },
  { "F9",             SDL_SCANCODE_F9 },
  { "F10",            SDL_SCANCODE_F10 },
  { "F11",            SDL_SCANCODE_F11 },
  { "F12",            SDL_SCANCODE_F12 },
  { "F13",            SDL_SCANCODE_F13 },
  { "F14",            SDL_SCANCODE_F14 },
  { "F15",            SDL_SCANCODE_F15 },

  // Modifier Keys
  // Removed Numlock, Capslock and Scrollock as don't seem to be handled well by SDL
  //{ "NUMLOCK",      SDLK_NUMLOCK },
  //{ "CAPSLOCK",     SDL_SCANCODE_CAPSLOCK },
  //{ "SCROLLLOCK",   SDLK_SCROLLOCK },
  { "RIGHTSHIFT",     SDL_SCANCODE_RSHIFT },
  { "LEFTSHIFT",      SDL_SCANCODE_LSHIFT },
  { "RIGHTCTRL",      SDL_SCANCODE_RCTRL },
  { "LEFTCTRL",       SDL_SCANCODE_LCTRL },
  { "RIGHTALT",       SDL_SCANCODE_RALT },
  { "LEFTALT",        SDL_SCANCODE_LALT },
  { "RIGHTMETA",      SDL_SCANCODE_RGUI },
  { "LEFTMETA",       SDL_SCANCODE_LGUI },
  { "ALTGR",          SDL_SCANCODE_MODE },

  // Other
  { "HELP",           SDL_SCANCODE_HELP },
  { "SYSREQ",         SDL_SCANCODE_SYSREQ },
  { "MENU",           SDL_SCANCODE_MENU },
  { "POWER",          SDL_SCANCODE_POWER },
  { "UNDO",           SDL_SCANCODE_UNDO }
};

CSDLInputSystem::CSDLInputSystem(const Util::Config::Node& config, bool useGameController)
  : CInputSystem("SDL"),
    m_keyState(nullptr),
    m_config(config),
    m_useGameController(useGameController),
    sdlConstForceMax(0),
    sdlSelfCenterMax(0),
    sdlFrictionMax(0),
    sdlVibrateMax(0)
{
  for (int i = 0; i < MAX_MICE; i++) {
      m_mouseX[i] = 0;
      m_mouseY[i] = 0;
      m_mouseZ[i] = 0;
      m_mouseButtons[i] = 0;
      m_mouseWheelDir[i] = 0;
  }
}

CSDLInputSystem::~CSDLInputSystem()
{
  CloseJoysticks();
}

void CSDLInputSystem::OpenJoysticks()
{
  // Open all available joysticks
  int numJoys = SDL_NumJoysticks();
  int numHapticAxes = 0;
  int possibleEffect = 0;

  //allow joystick to be fetch if windows has no focus.
  //this is usefull for multiple instance networked on the same machine
  SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");


  for (int joyNum = 0; joyNum < numJoys; joyNum++)
  {
    numHapticAxes = 0;

    SDL_GameController *gamepad = nullptr;
    SDL_Joystick *joystick = nullptr;

    if (m_useGameController)
    {
	gamepad = SDL_GameControllerOpen(joyNum);
	joystick = SDL_GameControllerGetJoystick(gamepad);
    } else
    {
	joystick = SDL_JoystickOpen(joyNum);
    }

    if (joystick == nullptr)
    {
      ErrorLog("Unable to open joystick device %d with SDL - skipping joystick.\n", joyNum + 1);
      continue;
    }

    // Gather joystick details (name, num POVs & buttons and which axes are available)
    JoyDetails joyDetails;
    hapticInfo hapticDatas;

    if (m_useGameController)
    {
      const char *pName = SDL_GameControllerName(gamepad);
      strncpy(joyDetails.name, pName, MAX_NAME_LENGTH);
      joyDetails.name[MAX_NAME_LENGTH] = '\0';
      joyDetails.numAxes = 6;
      joyDetails.numPOVs = 4;
      joyDetails.numButtons = 16;
    } else
    {
      const char *pName = SDL_JoystickName(joystick);
      strncpy(joyDetails.name, pName, MAX_NAME_LENGTH);
      joyDetails.name[MAX_NAME_LENGTH] = '\0';
      joyDetails.numAxes = SDL_JoystickNumAxes(joystick);
      joyDetails.numPOVs = SDL_JoystickNumHats(joystick);
      joyDetails.numButtons = SDL_JoystickNumButtons(joystick);
    }

    if (SDL_JoystickIsHaptic(joystick))
        joyDetails.hasFFeedback = true;
    else
        joyDetails.hasFFeedback = false;

    if (joyDetails.hasFFeedback)
    {
      hapticDatas.SDLhaptic = SDL_HapticOpenFromJoystick(joystick);

      if (hapticDatas.SDLhaptic == NULL)
      {
        ErrorLog("Unable to obtain haptic interface for joystick %s. Force feedback will be disabled for this joystick.", joyDetails.name);
        joyDetails.hasFFeedback = false;
        numHapticAxes = 0;
      }
      else
      {
        numHapticAxes = SDL_HapticNumAxes(hapticDatas.SDLhaptic);

        // depending device, SDL_HapticNumAxes return wrong number of ffb axes (bug on device driver on windows or other ?)
        // ie : saitek cyborg evo force joystick returns 3 ffb axes instead of 2 ffb axes, this leads to unable to create effects on this device
        // generally, none of commercial ffb products have more than 2 ffb axes
        // in this case, we need to force the correct number of ffb axes by adding manually a new sdl2 function
        // see https://forums.libsdl.org/viewtopic.php?t=5195
        // enabled this code if you have a saitek cyborg evo force or if you find another device that return wrong number of ffb axis
        // don't forget to edit sdl2 source code in Supermodel project
//#define HAPTIC_MOD
#if (defined _WIN32 && defined HAPTIC_MOD)
        if (numHapticAxes > 2)
        {
          DebugLog("Joystick : %s return more than 2 ffb axes, need sdl2 addon SDL_HapticSetAxes() to bypass\n", SDL_HapticName(joyNum));
          if (strcmp(joyDetails.name, "Saitek Cyborg Evo Force") == 0) // remove if any other particular case
            SDL_HapticSetAxes(hapticDatas.SDLhaptic, 2);   // /!\ function manually added to SDL2 project
        }
#endif


        // sdl2 bug on linux only ?
        // SDL_HapticNumAxes() : on win it reports good number of haptic axe, on linux 18.04 it reports always 2 haptic axes (bad) whatever the device
        // file : SDL2-2.0.10\src\haptic\linux\SDL_syshaptic.c
        // function : static int SDL_SYS_HapticOpenFromFD(SDL_Haptic * haptic, int fd)
        // haptic->naxes = 2;          // Hardcoded for now, not sure if it's possible to find out. <- note from the sdl2 devs
#ifndef _WIN32
        if (!HasBasicForce(hapticDatas.SDLhaptic)) numHapticAxes = 0;
#endif
        DebugLog("joy num %d haptic num axe %d name : %s\n", joyNum, numHapticAxes, SDL_HapticName(joyNum));
      }
    }

    for (int axisNum = 0; axisNum < NUM_JOY_AXES; axisNum++)
    {
      joyDetails.hasAxis[axisNum] = joyDetails.numAxes > axisNum;
      if (numHapticAxes > 0 && joyDetails.hasAxis[axisNum]) // unable to know which axes have ffb
        joyDetails.axisHasFF[axisNum] = true;
      else
        joyDetails.axisHasFF[axisNum] = false;

      char *axisName = joyDetails.axisName[axisNum];
      strcpy(axisName, CInputSystem::GetDefaultAxisName(axisNum));
    }

    if (joyDetails.hasFFeedback && hapticDatas.SDLhaptic != NULL && numHapticAxes > 0) // not a pad but wheel or joystick
    {
      SDL_HapticSetAutocenter(hapticDatas.SDLhaptic, 0);
      SDL_HapticSetGain(hapticDatas.SDLhaptic, 100);

      possibleEffect = SDL_HapticQuery(hapticDatas.SDLhaptic);

      // constant effect
      if (possibleEffect & SDL_HAPTIC_CONSTANT)
      {
        memset(&eff, 0, sizeof(SDL_HapticEffect));
        eff.type = SDL_HAPTIC_CONSTANT;
        eff.periodic.direction.type = SDL_HAPTIC_CARTESIAN;
        eff.constant.direction.dir[0] = 0;
        eff.constant.length = 30;
        eff.constant.delay = 0;
        eff.constant.level = 0;
        hapticDatas.effectConstantForceID = SDL_HapticNewEffect(hapticDatas.SDLhaptic, &eff);;
        if (hapticDatas.effectConstantForceID < 0)
          ErrorLog("Unable to create constant force effect for joystick %s (joy id=%d). Constant force will not be applied.", joyDetails.name, joyNum + 1);
      }

      // vibration effect
      if (possibleEffect & SDL_HAPTIC_SINE)
      {
        memset(&eff, 0, sizeof(SDL_HapticEffect));
        eff.type = SDL_HAPTIC_SINE;
        eff.periodic.direction.type = SDL_HAPTIC_CARTESIAN;
        eff.constant.length = 500;
        eff.constant.delay = 0;
        eff.periodic.period = 50;
        eff.periodic.magnitude = 0;
        hapticDatas.effectVibrationID = SDL_HapticNewEffect(hapticDatas.SDLhaptic, &eff);
        if (hapticDatas.effectVibrationID < 0)
          ErrorLog("Unable to create vibration effect for joystick %s (joy id=%d). Vibration will not be applied.", joyDetails.name, joyNum + 1);
      }

      // spring effect
      if (possibleEffect & SDL_HAPTIC_SPRING)
      {
        memset(&eff, 0, sizeof(SDL_HapticEffect));
        eff.type = SDL_HAPTIC_SPRING;
        eff.periodic.direction.type = SDL_HAPTIC_CARTESIAN;
        eff.condition.delay = 0;
        eff.condition.length = SDL_HAPTIC_INFINITY;
        eff.condition.left_sat[0] = 0xFFFF;
        eff.condition.right_sat[0] = 0xFFFF;
        eff.condition.left_coeff[0] = 0;
        eff.condition.right_coeff[0] = 0;
        hapticDatas.effectSpringForceID = SDL_HapticNewEffect(hapticDatas.SDLhaptic, &eff);
        if (hapticDatas.effectSpringForceID < 0)
          ErrorLog("Unable to create spring force effect for joystick %s (joy id=%d). Spring force will not be applied.", joyDetails.name, joyNum + 1);
      }

      // friction effect
      if (possibleEffect & SDL_HAPTIC_FRICTION)
      {
        memset(&eff, 0, sizeof(SDL_HapticEffect));
        eff.type = SDL_HAPTIC_FRICTION;
        eff.periodic.direction.type = SDL_HAPTIC_CARTESIAN;
        eff.condition.delay = 0;
        eff.condition.length = SDL_HAPTIC_INFINITY;
        eff.condition.left_sat[0] = 0xFFFF;
        eff.condition.right_sat[0] = 0xFFFF;
        eff.condition.left_coeff[0] = 0;
        eff.condition.right_coeff[0] = 0;
        hapticDatas.effectFrictionForceID = SDL_HapticNewEffect(hapticDatas.SDLhaptic, &eff);
        if (hapticDatas.effectFrictionForceID < 0)
          ErrorLog("Unable to create friction force effect for joystick %s (joy id=%d). Friction force will not be applied.", joyDetails.name, joyNum + 1);
      }
    }

    if (joyDetails.hasFFeedback && hapticDatas.SDLhaptic != NULL && numHapticAxes == 0) // pad with rumble. Note : SDL_HapticRumbleSupported() is not enough to detect rumble pad only because joystick or wheel may have also rumble
    {
      if (SDL_HapticRumbleInit(hapticDatas.SDLhaptic) < 0)
      {
        ErrorLog("Unable to create rumble effect for pad %s (pad id=%d). Rumble will not be applied.SDL_HapticRumbleInit failed : %s", joyDetails.name, joyNum + 1, SDL_GetError());
        joyDetails.axisHasFF[AXIS_X] = false;
        joyDetails.axisHasFF[AXIS_Y] = false;
      }
      else
      {
        joyDetails.axisHasFF[AXIS_X] = true; // Force feedback simulated on X axis sticks (fake)
        joyDetails.axisHasFF[AXIS_Y] = true; // Force feedback simulated on Y axis sticks (fake)
      }
    }

    if (m_useGameController)
        m_gamepads.push_back(gamepad);
    else
        m_joysticks.push_back(joystick);

    m_joyDetails.push_back(joyDetails);
    m_SDLHapticDatas.push_back(hapticDatas);
  }
}

void CSDLInputSystem::CloseJoysticks()
{
  // Close all previously opened joysticks
  for (int i = 0; i < GetNumJoysticks(); i++)
  {
    JoyDetails joyDetails = m_joyDetails[i];
    if (joyDetails.hasFFeedback)
    {
      if (m_SDLHapticDatas[i].effectConstantForceID >= 0)
        SDL_HapticDestroyEffect(m_SDLHapticDatas[i].SDLhaptic, m_SDLHapticDatas[i].effectConstantForceID);
      if (m_SDLHapticDatas[i].effectVibrationID >= 0)
        SDL_HapticDestroyEffect(m_SDLHapticDatas[i].SDLhaptic, m_SDLHapticDatas[i].effectVibrationID);
      if (m_SDLHapticDatas[i].effectSpringForceID >= 0)
        SDL_HapticDestroyEffect(m_SDLHapticDatas[i].SDLhaptic, m_SDLHapticDatas[i].effectSpringForceID);
      if (m_SDLHapticDatas[i].effectFrictionForceID >= 0)
        SDL_HapticDestroyEffect(m_SDLHapticDatas[i].SDLhaptic, m_SDLHapticDatas[i].effectFrictionForceID);

      SDL_HapticClose(m_SDLHapticDatas[i].SDLhaptic);
    }

    if (m_useGameController)
    {
      SDL_GameController *gamepad = m_gamepads[i];
      SDL_GameControllerClose(gamepad);
      m_gamepads.clear();
    } else {
      SDL_Joystick *joystick = m_joysticks[i];
      SDL_JoystickClose(joystick);
      m_joysticks.clear();
    }
  }

  m_joyDetails.clear();
  m_SDLHapticDatas.clear();

  for (int i = 0; i < MAX_MICE; i++) {
      m_mouseX[i] = 0;
      m_mouseY[i] = 0;
      m_mouseZ[i] = 0;
      m_mouseButtons[i] = 0;
      m_mouseWheelDir[i] = 0;
  }

  m_mseDetails.clear();
  ManyMouse_Quit();
}

bool CSDLInputSystem::InitializeSystem()
{
  // Make sure joystick subsystem is initialized and joystick events are enabled
  if (m_useGameController)
  {
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC) != 0)
    {
        ErrorLog("Unable to initialize SDL joystick subsystem (%s).\n", SDL_GetError());
        return false;
    }
    SDL_GameControllerEventState(SDL_ENABLE);
  } else {
    if (SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC) != 0)
    {
        ErrorLog("Unable to initialize SDL joystick subsystem (%s).\n", SDL_GetError());
        return false;
    }
    SDL_JoystickEventState(SDL_ENABLE);
  }

  // Open attached joysticks
  OpenJoysticks();

#ifdef SUPERMODEL_MANYMOUSE
  // Initiate ManyMouse
  available_mice = ManyMouse_Init(m_config["ABSMiceOnly"].ValueAs<bool>());
  static MouseDetails mice[MAX_MICE];

  std::cout << std::endl;

  for (int i = 0; i < available_mice; i++)
  {
     const char *name = ManyMouse_DeviceName(i);
     strncpy(mice[i].name, name, sizeof (mice[i].name));
     mice[i].name[sizeof (mice[i].name) - 1] = '\0';

     mice[i].isAbsolute = true;
     m_mseDetails.push_back(mice[i]);

     std::cout << "#" << i + 1 << ": " << mice[i].name << std::endl;
  }

  std::cout << "Found " << available_mice << " available mice"  << std::endl;
  std::cout << std::endl;
#endif

  return true;
}

int CSDLInputSystem::GetKeyIndex(const char *keyName)
{
  for (int i = 0; i < NUM_SDL_KEYS; i++)
  {
    if (stricmp(keyName, s_keyMap[i].keyName) == 0)
      return i;
  }
  return -1;
}

const char *CSDLInputSystem::GetKeyName(int keyIndex)
{
  if (keyIndex < 0 || keyIndex >= NUM_SDL_KEYS)
    return nullptr;
  return s_keyMap[keyIndex].keyName;
}

bool CSDLInputSystem::IsKeyPressed(int kbdNum, int keyIndex) const
{
  // Get SDL key for given index and check if currently pressed
  SDL_Keycode sdlKey = s_keyMap[keyIndex].sdlKey;
  return !!m_keyState[sdlKey];
}

int CSDLInputSystem::GetMouseAxisValue(int mseNum, int axisNum) const
{
  // Return value for given mouse axis
  switch (axisNum)
  {
    case AXIS_X: return m_mouseX[mseNum];
    case AXIS_Y: return m_mouseY[mseNum];
    case AXIS_Z: return m_mouseZ[mseNum];
    default:     return 0;
  }
}

int CSDLInputSystem::GetMouseWheelDir(int mseNum) const
{
  // Return wheel value
  return m_mouseWheelDir[mseNum];
}

bool CSDLInputSystem::IsMouseButPressed(int mseNum, int butNum) const
{
  // Return value for given mouse button
  switch (butNum)
  {
    case 0:  return !!(m_mouseButtons[mseNum] & SDL_BUTTON_LMASK);
#ifndef SUPERMODEL_MANYMOUSE
    case 1:  return !!(m_mouseButtons[mseNum] & SDL_BUTTON_MMASK);
    case 2:  return !!(m_mouseButtons[mseNum] & SDL_BUTTON_RMASK);
#else
    case 1:  return !!(m_mouseButtons[mseNum] & SDL_BUTTON_RMASK);
    case 2:  return !!(m_mouseButtons[mseNum] & SDL_BUTTON_MMASK);
#endif
    case 3:  return !!(m_mouseButtons[mseNum] & SDL_BUTTON_X1MASK);
    case 4:  return !!(m_mouseButtons[mseNum] & SDL_BUTTON_X2MASK);
    default: return false;
  }
}

int CSDLInputSystem::GetJoyAxisValue(int joyNum, int axisNum) const
{
  // Get raw joystick axis value for given joystick from SDL (values range from -32768 to 32767)
  if (m_useGameController)
  {
    SDL_GameController *gamepad = m_gamepads[joyNum];
    switch (axisNum) {
      case AXIS_X:  return SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_LEFTX);
      case AXIS_Y:  return SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_LEFTY);
      case AXIS_Z:  return SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
      case AXIS_RX: return SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_RIGHTX);
      case AXIS_RY: return SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_RIGHTY);
      case AXIS_RZ: return SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
      default:      return 0;
    }
  } else {
    SDL_Joystick *joystick = m_joysticks[joyNum];
    return SDL_JoystickGetAxis(joystick, axisNum);
  }
}

bool CSDLInputSystem::IsJoyPOVInDir(int joyNum, int povNum, int povDir) const
{
  // Get current joystick POV-hat value for given joystick and POV number from SDL and check if pointing in required direction
  if (m_useGameController)
  {
    SDL_GameController *gamepad = m_gamepads[joyNum];
    switch (povDir)
    {
      case POV_UP:    return SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_DPAD_UP);
      case POV_DOWN:  return SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
      case POV_LEFT:  return SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
      case POV_RIGHT: return SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
      default:        return false;
    }
    return false;
  } else {
    SDL_Joystick *joystick = m_joysticks[joyNum];
    int hatVal = SDL_JoystickGetHat(joystick, povNum);
    switch (povDir)
    {
      case POV_UP:    return !!(hatVal & SDL_HAT_UP);
      case POV_DOWN:  return !!(hatVal & SDL_HAT_DOWN);
      case POV_LEFT:  return !!(hatVal & SDL_HAT_LEFT);
      case POV_RIGHT: return !!(hatVal & SDL_HAT_RIGHT);
      default:        return false;
    }
    return false;
  }
}

bool CSDLInputSystem::IsJoyButPressed(int joyNum, int butNum) const
{
  // Get current joystick button state for given joystick and button number from SDL
  if (m_useGameController)
  {
    SDL_GameController *gamepad = m_gamepads[joyNum];
    switch (butNum)
    {
      case 0:  return SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_A);
      case 1:  return SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_B);
      case 2:  return SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_X);
      case 3:  return SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_Y);
      case 4:  return SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
      case 5:  return SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
      case 6:  return SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_BACK);
      case 7:  return SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_START);
      case 8:  return SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_LEFTSTICK);
      case 9:  return SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_RIGHTSTICK);
      case 10: return SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_PADDLE1);
      case 11: return SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_PADDLE2);
      case 12: return SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_GUIDE);
      case 13: return SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_PADDLE3);
      case 14: return SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_PADDLE4);
      case 15: return SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_MISC1);
      case 16: return SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_TOUCHPAD);
    }
    return false;
  }
  else
  {
    SDL_Joystick *joystick = m_joysticks[joyNum];
    return !!SDL_JoystickGetButton(joystick, butNum);
  }
}

bool CSDLInputSystem::ProcessForceFeedbackCmd(int joyNum, int axisNum, ForceFeedbackCmd ffCmd)
{
  switch (ffCmd.id)
  {
    case FFStop:
      StopAllEffect(joyNum);
      break;

    case FFConstantForce:
      sdlConstForceMax = m_config["SDLConstForceMax"].ValueAs<unsigned>();
      if (sdlConstForceMax == 0)
        return false;
      // note sr2 centering val=0.047244 and val=0.062992 alternatively (const value)
      //          max val between -1 to 1 (left right)
      if (ffCmd.force == 0.0f)
        StopConstanteforce(joyNum);
      else if (ffCmd.force > 0.0f)
        ConstantForceEffect(ffCmd.force * (float)(sdlConstForceMax / 100.0f), -1, SDL_HAPTIC_INFINITY, joyNum);
      else if (ffCmd.force < 0.0f)
        ConstantForceEffect(-ffCmd.force * (float)(sdlConstForceMax / 100.0f), 1, SDL_HAPTIC_INFINITY, joyNum);
      break;

    case FFSelfCenter:
      sdlSelfCenterMax = m_config["SDLSelfCenterMax"].ValueAs<unsigned>();
      if (sdlSelfCenterMax == 0)
        return false;
      SpringForceEffect(ffCmd.force * (float)(sdlSelfCenterMax / 100.0f), joyNum);
      break;

    case FFFriction:
      sdlFrictionMax = m_config["SDLFrictionMax"].ValueAs<unsigned>();
      if (sdlFrictionMax == 0)
        return false;
      FrictionForceEffect(ffCmd.force * (float)(sdlFrictionMax / 100.0f), joyNum);
      break;

    case FFVibrate:
      sdlVibrateMax = m_config["SDLVibrateMax"].ValueAs<unsigned>();
      if (sdlVibrateMax == 0)
        return false;
      VibrationEffect(ffCmd.force * (float)(sdlVibrateMax / 100.0f), joyNum);
      break;
    }
    return true;
}

int CSDLInputSystem::GetNumKeyboards() const
{
  // Return ANY_KEYBOARD as SDL 1.2 cannot handle multiple keyboards
  return ANY_KEYBOARD;
}

int CSDLInputSystem::GetNumMice() const
{
  return available_mice;
}

int CSDLInputSystem::GetNumJoysticks() const
{
  // Return number of joysticks found
  if (m_useGameController)
    return (int)m_gamepads.size();
  else
    return (int)m_joysticks.size();
}

const KeyDetails *CSDLInputSystem::GetKeyDetails(int kbdNum)
{
  // Return nullptr as SDL 1.2 cannot handle multiple keyboards
  return nullptr;
}

const MouseDetails *CSDLInputSystem::GetMouseDetails(int mseNum)
{
  return &m_mseDetails[mseNum];
}

const JoyDetails *CSDLInputSystem::GetJoyDetails(int joyNum)
{
  return &m_joyDetails[joyNum];
}

bool CSDLInputSystem::Poll()
{
  // Reset mouse wheel direction
  for (int i = 0; i < available_mice; i++) {
     m_mouseWheelDir[i] = 0;
  }

  // Poll for event from SDL
  SDL_Event e;
  while (SDL_PollEvent(&e))
  {
    switch (e.type)
		{
	  default:
	    break;
		case SDL_QUIT:
			return false;
#ifndef SUPERMODEL_MANYMOUSE
		case SDL_MOUSEWHEEL:
			if (e.button.y > 0)
			{
				m_mouseZ[0] += 5;
				m_mouseWheelDir[0] = 1;
			}
			else if (e.button.y < 0)
			{
				m_mouseZ[0] -= 5;
				m_mouseWheelDir[0] = -1;
			}
			break;
#endif
		}
  }

  // Get key state from SDL
  m_keyState = SDL_GetKeyboardState(nullptr);

#ifdef SUPERMODEL_MANYMOUSE
  // Get mouse states from ManyMouse...
  while (ManyMouse_PollEvent(&mm_event)) {

      if (mm_event.device >= (unsigned int) available_mice)
          continue;

      Mouse *mouse;
      static Mouse mice[MAX_MICE];
      mouse = &mice[mm_event.device];
      int max_width = get_total_width();
      int max_height = get_total_height();
      float val, maxval;

      switch(mm_event.type) {
	  case MANYMOUSE_EVENT_BUTTON:
		if (mm_event.value == 1) {
			mouse->buttons |= (1 << mm_event.item);
		}
		else {
			mouse->buttons &= ~(1 << mm_event.item);
		}
		m_mouseButtons[mm_event.device] = (Uint32)mouse->buttons;
		break;
	  case MANYMOUSE_EVENT_RELMOTION:
		if (mm_event.item == 0) {
			mouse->x += mm_event.value;
			if (mouse->x < 0) mouse->x = 0;
			else if (mouse->x >= max_width) mouse->x = max_width;

			m_mouseX[mm_event.device] = mouse->x;
		}
		else if (mm_event.item == 1) {
			mouse->y += mm_event.value;
			if (mouse->y < 0) mouse->y = 0;
			else if (mouse->y >= max_height) mouse->y = max_height;

			m_mouseY[mm_event.device] = mouse->y;
		}
		break;
	  case MANYMOUSE_EVENT_ABSMOTION:
		val = (float) (mm_event.value - mm_event.minval);
		maxval = (float) (mm_event.maxval - mm_event.minval);

		if (mm_event.item == 0) {
			mouse->x = (val / maxval) * max_width;
			m_mouseX[mm_event.device] = mouse->x;
		}
		else if (mm_event.item == 1) {
			mouse->y = (val / maxval) * max_height;
			m_mouseY[mm_event.device] = mouse->y;
		}
		break;
	  case MANYMOUSE_EVENT_SCROLL:
		if (mm_event.item == 0)
		{
			if (mm_event.value > 0) {
				if (m_mouseZ[mm_event.device] > abs(0x5f))
				    m_mouseZ[mm_event.device] = abs(0x5f);

				m_mouseZ[mm_event.device] += 5;
				m_mouseWheelDir[mm_event.device] = 1;
			} else {
				if (m_mouseZ[mm_event.device] < -abs(0x5f))
				    m_mouseZ[mm_event.device] = -abs(0x5f);

				m_mouseZ[mm_event.device] -= 5;
				m_mouseWheelDir[mm_event.device] = -1;
			}
		}
		break;
	  default:
		break;
      }
  }
#else
	m_mouseButtons[0] = SDL_GetMouseState(&m_mouseX[0], &m_mouseY[0]);
#endif

  return true;
}

void CSDLInputSystem::SetMouseVisibility(bool visible)
{
  SDL_ShowCursor(visible ? SDL_ENABLE : SDL_DISABLE);
}

void CSDLInputSystem::StopAllEffect(int joyNum)
{
  StopConstanteforce(joyNum);
  StopVibrationforce(joyNum);
  StopSpringforce(joyNum);
  StopFrictionforce(joyNum);
}

void CSDLInputSystem::StopConstanteforce(int joyNum)
{
  // stop constante effect or rumble constant effect
  memset(&eff, 0, sizeof(SDL_HapticEffect));
  eff.type = SDL_HAPTIC_CONSTANT;
  eff.periodic.direction.type = SDL_HAPTIC_CARTESIAN;
  eff.constant.direction.dir[0] = 0;
  eff.constant.length = 30;
  eff.constant.delay = 0;
  eff.constant.level = 0;

  if (SDL_HapticEffectSupported(m_SDLHapticDatas[joyNum].SDLhaptic, &eff))
  {
    SDL_HapticUpdateEffect(m_SDLHapticDatas[joyNum].SDLhaptic, m_SDLHapticDatas[joyNum].effectConstantForceID, &eff);
  }
  else
  {
    SDL_HapticRumbleStop(m_SDLHapticDatas[joyNum].SDLhaptic);
  }
}

void CSDLInputSystem::StopVibrationforce(int joyNum)
{
  // stop vibration-rumble effect
  memset(&eff, 0, sizeof(SDL_HapticEffect));
  eff.type = SDL_HAPTIC_SINE;
  eff.periodic.direction.type = SDL_HAPTIC_CARTESIAN;
  eff.constant.length = 500;
  eff.constant.delay = 0;
  eff.periodic.period = 50;
  eff.periodic.magnitude = 0;

  if (SDL_HapticEffectSupported(m_SDLHapticDatas[joyNum].SDLhaptic, &eff))
  {
    SDL_HapticUpdateEffect(m_SDLHapticDatas[joyNum].SDLhaptic, m_SDLHapticDatas[joyNum].effectVibrationID, &eff);
  }
  else
  {
    SDL_HapticRumbleStop(m_SDLHapticDatas[joyNum].SDLhaptic);
  }

}

void CSDLInputSystem::StopSpringforce(int joyNum)
{
  // stop spring effect
  memset(&eff, 0, sizeof(SDL_HapticEffect));
  eff.type = SDL_HAPTIC_SPRING;
  eff.periodic.direction.type = SDL_HAPTIC_CARTESIAN;
  eff.condition.delay = 0;
  eff.condition.length = SDL_HAPTIC_INFINITY;
  eff.condition.left_sat[0] = 0xFFFF;
  eff.condition.right_sat[0] = 0xFFFF;
  eff.condition.left_coeff[0] = 0;
  eff.condition.right_coeff[0] = 0;

  if (SDL_HapticEffectSupported(m_SDLHapticDatas[joyNum].SDLhaptic, &eff))
  {
    SDL_HapticUpdateEffect(m_SDLHapticDatas[joyNum].SDLhaptic, m_SDLHapticDatas[joyNum].effectSpringForceID, &eff);
  }

}

void CSDLInputSystem::StopFrictionforce(int joyNum)
{
  // stop friction effect
  memset(&eff, 0, sizeof(SDL_HapticEffect));
  eff.type = SDL_HAPTIC_FRICTION;
  eff.periodic.direction.type = SDL_HAPTIC_CARTESIAN;
  eff.condition.delay = 0;
  eff.condition.length = SDL_HAPTIC_INFINITY;
  eff.condition.left_sat[0] = 0xFFFF;
  eff.condition.right_sat[0] = 0xFFFF;
  eff.condition.left_coeff[0] = 0;
  eff.condition.right_coeff[0] = 0;

  if (SDL_HapticEffectSupported(m_SDLHapticDatas[joyNum].SDLhaptic, &eff))
  {
    SDL_HapticUpdateEffect(m_SDLHapticDatas[joyNum].SDLhaptic, m_SDLHapticDatas[joyNum].effectFrictionForceID, &eff);
  }

}

void CSDLInputSystem::VibrationEffect(float strength, int joyNum)
{
  memset(&eff, 0, sizeof(SDL_HapticEffect));
  eff.type = SDL_HAPTIC_SINE;
  eff.constant.direction.type = SDL_HAPTIC_CARTESIAN;
  eff.periodic.delay = 0;
  eff.periodic.length = SDL_HAPTIC_INFINITY;
  eff.periodic.period = 50;
  eff.periodic.magnitude = (int)(strength * INT16_MAX);

  if (SDL_HapticEffectSupported(m_SDLHapticDatas[joyNum].SDLhaptic, &eff))
  {
    SDL_HapticUpdateEffect(m_SDLHapticDatas[joyNum].SDLhaptic, m_SDLHapticDatas[joyNum].effectVibrationID, &eff);

    SDL_HapticRunEffect(m_SDLHapticDatas[joyNum].SDLhaptic, m_SDLHapticDatas[joyNum].effectVibrationID, 1);
  }
  else
  {
    if (strength != 0.0f)
      SDL_HapticRumblePlay(m_SDLHapticDatas[joyNum].SDLhaptic, strength, 0);
    else
      SDL_HapticRumbleStop(m_SDLHapticDatas[joyNum].SDLhaptic);
  }

}

void CSDLInputSystem::ConstantForceEffect(float force, int dir, int length, int joyNum)
{
  memset(&eff, 0, sizeof(SDL_HapticEffect));
  eff.type = SDL_HAPTIC_CONSTANT;
  eff.constant.direction.type = SDL_HAPTIC_CARTESIAN;
  eff.constant.direction.dir[0] = 0; // in cartesian mode dir on x set 0 on y set 1
  eff.constant.length = length;
  eff.constant.level = (int)(dir * (force * INT16_MAX));

  if (SDL_HapticEffectSupported(m_SDLHapticDatas[joyNum].SDLhaptic, &eff))
  {
    SDL_HapticUpdateEffect(m_SDLHapticDatas[joyNum].SDLhaptic, m_SDLHapticDatas[joyNum].effectConstantForceID, &eff);

    SDL_HapticRunEffect(m_SDLHapticDatas[joyNum].SDLhaptic, m_SDLHapticDatas[joyNum].effectConstantForceID, 1);
  }
  else
  {
    float threshold = (float)m_config["SDLConstForceThreshold"].ValueAs<unsigned>() / 100.0f;
    if (force != 0.0f && force > threshold)
      SDL_HapticRumblePlay(m_SDLHapticDatas[joyNum].SDLhaptic, force, 200);
    else
      SDL_HapticRumbleStop(m_SDLHapticDatas[joyNum].SDLhaptic);
  }

}

void CSDLInputSystem::SpringForceEffect(float force, int joyNum)
{
  memset(&eff, 0, sizeof(SDL_HapticEffect));
  eff.type = SDL_HAPTIC_SPRING;
  eff.constant.direction.type = SDL_HAPTIC_CARTESIAN;
  eff.condition.delay = 0;
  eff.condition.length = SDL_HAPTIC_INFINITY;
  eff.condition.left_sat[0] = 0xffff;
  eff.condition.right_sat[0] = 0xffff;
  eff.condition.left_coeff[0] = (int)(force * INT16_MAX);
  eff.condition.right_coeff[0] = (int)(force * INT16_MAX);

  if (SDL_HapticEffectSupported(m_SDLHapticDatas[joyNum].SDLhaptic, &eff))
  {
    SDL_HapticUpdateEffect(m_SDLHapticDatas[joyNum].SDLhaptic, m_SDLHapticDatas[joyNum].effectSpringForceID, &eff);

    SDL_HapticRunEffect(m_SDLHapticDatas[joyNum].SDLhaptic, m_SDLHapticDatas[joyNum].effectSpringForceID, 1);
  }

}

void CSDLInputSystem::FrictionForceEffect(float force, int joyNum)
{
  memset(&eff, 0, sizeof(SDL_HapticEffect));
  eff.type = SDL_HAPTIC_FRICTION;
  eff.constant.direction.type = SDL_HAPTIC_CARTESIAN;
  eff.condition.delay = 0;
  eff.condition.length = SDL_HAPTIC_INFINITY;
  eff.condition.left_sat[0] = 0xffff;
  eff.condition.right_sat[0] = 0xffff;
  eff.condition.left_coeff[0] = (int)(force * INT16_MAX);
  eff.condition.right_coeff[0] = (int)(force * INT16_MAX);

  if (SDL_HapticEffectSupported(m_SDLHapticDatas[joyNum].SDLhaptic, &eff))
  {
    SDL_HapticUpdateEffect(m_SDLHapticDatas[joyNum].SDLhaptic, m_SDLHapticDatas[joyNum].effectFrictionForceID, &eff);

    SDL_HapticRunEffect(m_SDLHapticDatas[joyNum].SDLhaptic, m_SDLHapticDatas[joyNum].effectFrictionForceID, 1);
  }

}

// due to the sdl2 SDL_HapticNumAxes() bug in linux (always return 2 in linux)
// test if haptic controller has the most basic constant force effect
// if it has -> ffb wheel or ffb joystick
// if it hasn't -> pad
bool CSDLInputSystem::HasBasicForce(SDL_Haptic* hap)
{
  memset(&eff, 0, sizeof(SDL_HapticEffect));
  eff.type = SDL_HAPTIC_CONSTANT;
  eff.periodic.direction.type = SDL_HAPTIC_CARTESIAN;
  eff.constant.direction.dir[0] = 0;
  eff.constant.length = 30;
  eff.constant.delay = 0;
  eff.constant.level = 0;

  if (SDL_HapticEffectSupported(hap, &eff))
    return true;
  else
    return false;
}
