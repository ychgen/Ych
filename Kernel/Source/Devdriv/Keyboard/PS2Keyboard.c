#include "Devdriv/Keyboard/PS2Keyboard.h"

#include "./PS2Scancodes.h"

#include "ACPI/PCIe.h"

#include "Devdriv/Device.h"

#include "CPU/Interrupt.h"
#include "CPU/PortIO.h"
#include "CPU/PerCpu.h"

#include "HumanInterface/KeyboardHub.h"

#include "Intmgmt/IRQAldist.h"
#include "Intmgmt/IOAPIC.h"

#include "KRTL/Krnlmem.h"

#include "Earlyvideo/DisplaywideTextProtocol.h"

#define PS2_PORT_DATA   0x60 // R/W
#define PS2_PORT_STATUS 0x64 // R
#define PS2_PORT_CMD    0x64 // W

#define PS2_STATUS_OUTPUT   (1 << 0)
#define PS2_STATUS_INPUT    (1 << 1)
#define PS2_STATUS_SYSTEM   (1 << 2)
#define PS2_STATUS_CMDDATA  (1 << 3) // Command(0)/Data(1)
#define PS2_STATUS_ETIMEOUT (1 << 6)
#define PS2_STATUS_EPARITY  (1 << 7)

#define PS2_CONT_CFG_TRANSLATE (1 << 6)

#define PS2_CMD_READ_CONT_CFG_BYTE  0x20
#define PS2_CMD_WRITE_CONT_CFG_BYTE 0x60
#define PS2_CMD_GETSET_SCANSET      0xF0
#define PS2_CMD_GETSET_SCANSET_GET  0x00
#define PS2_CMD_GETSET_SCANSET_SET1 0x01
#define PS2_CMD_GETSET_SCANSET_SET2 0x02
#define PS2_CMD_GETSET_SCANSET_SET3 0x03

#define PS2_RESPONSE_ACK    0xFA
#define PS2_RESPONSE_RESEND 0xFE

#define PS2_KEYBOARD_IRQ    0x1

STATIC BOOL PS2_KeyboardDriverRegister(VOID);
STATIC VOID PS2_KeyboardDriverUnregister(VOID);

STATIC VOID PS2_WaitInputBufferEmpty(VOID);
STATIC VOID PS2_WaitOutputBufferEmpty(VOID);
STATIC BYTE PS2_IssueCommand(const BYTE* CommandStream, UINT NrBytes, BOOL ToCmd);

STATIC VOID PS2_KeyboardInterrupt(CONST InterruptFrame* Intfr);
STATIC KeyMod PS2_SampleKeymod(VOID);

STATIC CONST CHAR PS2KeyboardDeviceName[] = "PS/2 Keyboard";
STATIC DeviceNode PS2KeyboardDevice = { 0 };

STATIC KeyState PS2_StandardKeyStates[256] = {0}; // PRESSED or RELEASED
STATIC KeyState PS2_ExtendedKeyStates[256] = {0}; // PRESSED or RELEASED

//
#define LISTEN_EXTENDED  (1U << 1)
#define LISTEN_BROKEN    (1U << 2)
#define LISTEN_PRTSCAS   (1U << 3)
#define LISTEN_PRTSCDEAS (1U << 4)
#define LISTEN_PAUSE     (1U << 5)
//
STATIC UINT Listen    = 0;
STATIC UINT ListenIdx = 0;
//

KeyboardDeviceDriver g_PS2_KeyboardDriver = {
    .Object = {
        .Name = "PS/2 Keyboard Driver",
        .Type = DRIVER_KEYBOARD,
        .pInterface = &g_PS2_KeyboardDriver,
        .pMatchMethods = NULLPTR,
        .Register = PS2_KeyboardDriverRegister,
        .Unregister = PS2_KeyboardDriverUnregister,
        .DevProbe = NULLPTR,
        .DevInit = NULLPTR,
        .DevShutdown = NULLPTR
    },
    .GetKeyStatus = PS2_KeyboardGetKeyState
};

STATIC BOOL PS2_KeyboardDriverRegister(VOID)
{
    STATIC BOOL DeviceInitialized = FALSE;
    PS2KeyboardDevice.pDriver = &g_PS2_KeyboardDriver.Object;

    if (!DeviceInitialized)
    {
        KrtlContiguousCopyBuffer(PS2KeyboardDevice.Name, PS2KeyboardDeviceName, sizeof(PS2KeyboardDeviceName));
        PS2KeyboardDevice.OnBus = BUS_PLATFORM;

        AddChildDevice(GetSystemRootDevice(), &PS2KeyboardDevice);
        DeviceInitialized = TRUE;
    }

    {
        BYTE CMD = PS2_CMD_READ_CONT_CFG_BYTE;
        BYTE CCB = PS2_IssueCommand(&CMD, 1, TRUE);
        CCB &= ~PS2_CONT_CFG_TRANSLATE;

        PS2_WaitInputBufferEmpty();
        ByteOutputToPort(PS2_PORT_CMD, PS2_CMD_WRITE_CONT_CFG_BYTE);
        PS2_WaitInputBufferEmpty();
        ByteOutputToPort(PS2_PORT_DATA, CCB);

        CONST BYTE SetScanset2[] = { PS2_CMD_GETSET_SCANSET, PS2_CMD_GETSET_SCANSET_SET2 };
        if (PS2_IssueCommand(SetScanset2, sizeof(SetScanset2), FALSE) != PS2_RESPONSE_ACK)
        {
            return FALSE;
        }
    }

    PairOfProcessorIntVector Offer = AldistOfferSetupForIRQ(PS2_KEYBOARD_IRQ);
    SetHardwareInterrupt(Offer.CpuID, Offer.IntVector, &PS2KeyboardDevice, PS2_KEYBOARD_IRQ, PS2_KeyboardInterrupt);
    SetRedirectIRQ(PS2_KEYBOARD_IRQ, Offer.CpuID, Offer.IntVector);
    
    return TRUE;
}

STATIC VOID PS2_KeyboardDriverUnregister(VOID)
{
    SetEnableIRQ(PS2_KEYBOARD_IRQ, FALSE);
}

STATIC VOID PS2_WaitInputBufferEmpty(VOID)
{
    while (ByteInputFromPort(PS2_PORT_STATUS) & PS2_STATUS_INPUT);
}

STATIC VOID PS2_WaitOutputBufferEmpty(VOID)
{
    while (!(ByteInputFromPort(PS2_PORT_STATUS) & PS2_STATUS_OUTPUT));
}

STATIC BYTE PS2_IssueCommand(const BYTE* CommandStream, UINT NrBytes, BOOL ToCmd)
{
    CONST UINT MAX_RESENDS = 3;
    BYTE Response;

    for (UINT i = 0, NrResends = 0; i < NrBytes && NrResends < MAX_RESENDS; i++)
    {
        PS2_WaitInputBufferEmpty();
        ByteOutputToPort((i == 0 && ToCmd) ? PS2_PORT_CMD : PS2_PORT_DATA, CommandStream[i]);
        
        if (ToCmd)
        {
            PS2_WaitInputBufferEmpty();
        }
        else
        {
            PS2_WaitOutputBufferEmpty();
        }

        Response = ByteInputFromPort(PS2_PORT_DATA);
        if (Response == PS2_RESPONSE_RESEND)
        {
            i = 0;
            NrResends++;
        }
    }

    return Response;
}

STATIC VOID PS2_KeyboardInterrupt(CONST InterruptFrame* Intfr)
{
    CONST UCHAR Data = ByteInputFromPort(PS2_PORT_DATA);

    if (Listen & LISTEN_PRTSCAS)
    {
        if (ListenIdx++ == 3)
        {
            HiPushKeyboardEvent((HiKeyboardEvent) {
                .Keycode = KC_PRINT_SCREEN,
                .OldState = KS_RELEASED,
                .NewState = KS_PRESSED,
                .Keymod = PS2_SampleKeymod(),
                .Source = &PS2KeyboardDevice
            });
            PS2_StandardKeyStates[0xF7] = KS_PRESSED;
            Listen = ListenIdx = 0;
            return;
        }
        return;
    }
    else if (Listen & LISTEN_PRTSCDEAS)
    {
        if (ListenIdx++ == 5)
        {
            HiPushKeyboardEvent((HiKeyboardEvent) {
                .Keycode = KC_PRINT_SCREEN,
                .OldState = KS_PRESSED,
                .NewState = KS_RELEASED,
                .Keymod = PS2_SampleKeymod(),
                .Source = &PS2KeyboardDevice
            });
            PS2_StandardKeyStates[0xF7] = KS_RELEASED;
            Listen = ListenIdx = 0;
            return;
        }
        return;
    }
    else if (Listen & LISTEN_PAUSE)
    {
        if (ListenIdx++ == 7)
        {
            HiPushKeyboardEvent((HiKeyboardEvent) {
                .Keycode = KC_PAUSE,
                .OldState = KS_RELEASED,
                .NewState = KS_PRESSED,
                .Keymod = PS2_SampleKeymod(),
                .Source = &PS2KeyboardDevice
            });
            HiPushKeyboardEvent((HiKeyboardEvent) {
                .Keycode = KC_PAUSE,
                .OldState = KS_PRESSED,
                .NewState = KS_RELEASED,
                .Keymod = PS2_SampleKeymod(),
                .Source = &PS2KeyboardDevice
            });
            Listen = ListenIdx = 0;
            return;
        }
        return;
    }

    switch (Data)
    {
    case PS2_XSCANCODE:
    {
        Listen |= LISTEN_EXTENDED;
        ListenIdx++;
        return;
    }
    case PS2_BREAK_PREFIX:
    {
        Listen |= LISTEN_BROKEN;
        ListenIdx++;
        return;
    }
    case 0x12:
    {
        if (ListenIdx == 1 && (Listen & LISTEN_EXTENDED))
        {
            Listen |= LISTEN_PRTSCAS;
            ListenIdx++;
            return;
        }
        break;
    }
    case 0x7C:
    {
        if (ListenIdx == 2 && (Listen & (LISTEN_EXTENDED | LISTEN_BROKEN)) == (LISTEN_EXTENDED | LISTEN_BROKEN))
        {
            Listen |= LISTEN_PRTSCDEAS;
            ListenIdx++;
            return;
        }
        break;
    }
    case 0xE1:
    {
        Listen |= LISTEN_PAUSE;
        ListenIdx++;
        return;
    }
    default:
    {
        break;
    }
    }
    CONST KeyState KsOld = Listen & LISTEN_BROKEN ? KS_PRESSED  : KS_RELEASED;
    CONST KeyState KsNew = Listen & LISTEN_BROKEN ? KS_RELEASED : KS_PRESSED;

    ((Listen & LISTEN_EXTENDED) ? PS2_ExtendedKeyStates : PS2_StandardKeyStates)[Data] = KsNew;

    HiPushKeyboardEvent((HiKeyboardEvent) {
        .Keycode = PS2_ScancodeToKeycode(Listen & LISTEN_EXTENDED, Data),
        .OldState = KsOld,
        .NewState = KsNew,
        .Keymod   = PS2_SampleKeymod(),
        .Source   = &PS2KeyboardDevice
    });
    Listen = 0;
    ListenIdx = 0;
}

STATIC KeyMod PS2_SampleKeymod(VOID)
{
    KeyMod Keymod = 0;
    if (PS2_StandardKeyStates[PS2_SCANCODE_LCTRL] == KS_PRESSED || PS2_ExtendedKeyStates[PS2_XSCANCODE_RCTRL] == KS_PRESSED)
    {
        Keymod |= KM_CTRL;
    }
    if (PS2_StandardKeyStates[PS2_SCANCODE_LALT] == KS_PRESSED || PS2_ExtendedKeyStates[PS2_XSCANCODE_RALT] == KS_PRESSED)
    {
        Keymod |= KM_ALT;
    }
    if (PS2_StandardKeyStates[PS2_SCANCODE_LSHIFT] == KS_PRESSED || PS2_StandardKeyStates[PS2_SCANCODE_RSHIFT] == KS_PRESSED)
    {
        Keymod |= KM_SHIFT;
    }
    return Keymod;
}

KeyState PS2_KeyboardGetKeyState(KeyCode Keycode)
{
    BOOL bExtended;
    UCHAR Scancode = PS2_KeycodeToScancode(Keycode, &bExtended);

    if (Scancode == 0)
    {
        return KS_RELEASED;
    }

    return bExtended ? PS2_ExtendedKeyStates[Scancode] : PS2_StandardKeyStates[Scancode];
}
