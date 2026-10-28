#include "Devdriv/Device.h"

#include "Core/KernelState.h"

#include "Earlyvideo/DisplaywideTextProtocol.h"

#include "Memory/Krnlheap.h"
#include "KRTL/Krnlstring.h"
#include "KRTL/Krnlmem.h"

#include "ACPI/PCIe.h"

static const CHAR c_RootDeviceName[] = "System Host";
static const CHAR c_PcieDeviceName[] = "ACPI PCIe Root 0";
static DeviceNode g_RootDevice = {0};
static DeviceNode g_PcieRoot = {0};
static UINT g_DeviceCount = 0;

static VOID DevTreeInitPCIeDeviceFound(UINTPTR VaddrCfgSpace, WORD VendorID, WORD DeviceID, UCHAR Class, UCHAR SubClass, UCHAR Bus, UCHAR Device, UCHAR Function, UCHAR ProgIF);

VOID InitDeviceTree(VOID)
{
    if (!InitDeviceTreeRoot())
    {
        return;
    }

    KrtlContiguousZeroBuffer(&g_PcieRoot, sizeof(DeviceNode));
    KrtlContiguousCopyBuffer(g_PcieRoot.Name, c_PcieDeviceName, sizeof(c_PcieDeviceName));
    
    g_PcieRoot.OnBus = BUS_PCI;
    g_PcieRoot.Pci.VaddrCfgSpace = PcieGetVoffset(0, 0, 0, 0);

    AddChildDevice(&g_RootDevice, &g_PcieRoot);
    g_DeviceCount++;

    PcieEnumerate(DevTreeInitPCIeDeviceFound);
}

BOOL InitDeviceTreeRoot(VOID)
{
    if (g_RootDevice.OnBus != BUS_IVLD)
    {
        return FALSE;
    }

    KrtlContiguousZeroBuffer(&g_RootDevice, sizeof(DeviceNode));
    KrtlContiguousCopyBuffer(g_RootDevice.Name, c_RootDeviceName, sizeof(c_RootDeviceName));
    g_RootDevice.OnBus = BUS_ROOT;

    g_DeviceCount = 1;
    return TRUE;
}

VOID AddChildDevice(DeviceNode* pParent, DeviceNode* pChild)
{
    if (!(pParent && pChild))
    {
        return;
    }

    pChild->pParent = pParent;
    pChild->pNxtSib = NULLPTR;

    if (!pParent->p1stKid)
    {
        pParent->p1stKid = pChild;
    }
    else
    {
        DeviceNode* pSib = pParent->p1stKid;
        while (pSib->pNxtSib)
        {
            pSib = pSib->pNxtSib;
        }
        pSib->pNxtSib = pChild;
    }
}

DeviceNode* GetSystemRootDevice(VOID)
{
    return &g_RootDevice;
}

UINT GetSystemDeviceCount(VOID)
{
    return g_DeviceCount;
}

static VOID DevTreeInitPCIeDeviceFound(UINTPTR VaddrCfgSpace, WORD VendorID, WORD DeviceID, UCHAR Class, UCHAR SubClass, UCHAR Bus, UCHAR Device, UCHAR Function, UCHAR ProgIF)
{
    DeviceNode* pDevice = (DeviceNode*) HeapAcquire(sizeof(DeviceNode));
    if (!pDevice)
    {
        return;
    }
    KrtlContiguousZeroBuffer(pDevice, sizeof(DeviceNode));

    pDevice->OnBus = BUS_PCI;
    pDevice->Pci.VaddrCfgSpace = VaddrCfgSpace;
    pDevice->Pci.VendorID = VendorID;
    pDevice->Pci.DeviceID = DeviceID;
    pDevice->Pci.Bus = Bus;
    pDevice->Pci.Device = Device;
    pDevice->Pci.Function = Function;
    pDevice->Pci.ProgIF = ProgIF;
    pDevice->Pci.Class = Class;
    pDevice->Pci.SubClass = SubClass;
    
    CSTR StrDefaultName = PcieGetClassString(Class);
    KrtlContiguousCopyBuffer(pDevice->Name, StrDefaultName, KrtlStringSize(StrDefaultName));

    AddChildDevice(&g_PcieRoot, pDevice);
    g_DeviceCount++;
}
