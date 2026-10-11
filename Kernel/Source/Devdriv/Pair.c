#include "Devdriv/Pair.h"

#include "Core/KernelState.h"

#include "Earlyvideo/DisplaywideTextProtocol.h"

static BOOL TryPairDeviceWithAnyDriver(DeviceNode* pDevice);
static UINT LocalDevdrivPairAttempt(DeviceNode* pRootNode);

BOOL IsDriverCompatibleWithDevice(Driver* pDriver, DeviceNode* pDevice, DriverMatchMethod* pOutMatchMethod)
{
    for (DriverMatchMethod* pMethod = pDriver->pMatchMethods; pMethod->OnBus != BUS_IVLD; pMethod++)
    {
        if (pDevice->OnBus != pMethod->OnBus)
        {
            continue;
        }

        switch (pMethod->OnBus)
        {
        case BUS_PCI:
        {
        #define PairEnsureNot(expr) if ((expr)) { continue; }
            PairEnsureNot(pMethod->Pci.VendorID != PCI_ANY_VENDOR_ID && pMethod->Pci.VendorID != pDevice->Pci.VendorID);
            PairEnsureNot(pMethod->Pci.DeviceID != PCI_ANY_DEVICE_ID && pMethod->Pci.DeviceID != pDevice->Pci.DeviceID);
            PairEnsureNot(pMethod->Pci.Bus != PCI_ANY_BUS && pMethod->Pci.Bus != pDevice->Pci.Bus);
            PairEnsureNot(pMethod->Pci.Device != PCI_ANY_DEVICE && pMethod->Pci.Device != pDevice->Pci.Device);
            PairEnsureNot(pMethod->Pci.Function != PCI_ANY_FUNCTION && pMethod->Pci.Function != pDevice->Pci.Function);
            PairEnsureNot(pMethod->Pci.ProgIF != PCI_ANY_PROG_IF && pMethod->Pci.ProgIF != pDevice->Pci.ProgIF);
            PairEnsureNot(pMethod->Pci.Class != PCI_ANY_CLASS && pMethod->Pci.Class != pDevice->Pci.Class);
            PairEnsureNot(pMethod->Pci.SubClass != PCI_ANY_SUBCLASS && pMethod->Pci.SubClass != pDevice->Pci.SubClass);
        #undef PairEnsureNot
            if (pOutMatchMethod)
            {
                *pOutMatchMethod = *pMethod;
            }
            return TRUE;
        }
        default: continue;
        }
    }
    return FALSE;
}

BOOL TryPairDriverWithDevice(Driver* pDriver, DeviceNode* pDevice)
{
    if (!(pDriver && pDevice && !pDevice->pDriver))
    {
        return FALSE;
    }
    DriverMatchMethod MatchMethod;
    if (!IsDriverCompatibleWithDevice(pDriver, pDevice, &MatchMethod))
    {
        return FALSE;
    }
    if (!pDriver->DevProbe(pDevice, &MatchMethod))
    {
        return FALSE;
    }

    pDevice->pDriver = pDriver;
    if (!pDriver->DevInit(pDevice))
    {
        pDevice->pDriver = NULLPTR;
        return FALSE;
    }

    return TRUE;
}

UINT GlobalDevdrivPairAttempt(VOID)
{
    return LocalDevdrivPairAttempt(GetSystemRootDevice());
}

static BOOL TryPairDeviceWithAnyDriver(DeviceNode* pDevice)
{
    for (UINT i = 0; i < g_KernelState.DriverInfo.NrLoaded; i++)
    {
        if (TryPairDriverWithDevice(g_KernelState.DriverInfo.pTable[i], pDevice))
        {
            return TRUE;
        }
    }
    return FALSE;
}

static UINT LocalDevdrivPairAttempt(DeviceNode* pRootNode)
{
    if (!pRootNode)
    {
        return 0;
    }
    UINT NrPaired = 0;
    
    while (pRootNode)
    {
        if (TryPairDeviceWithAnyDriver(pRootNode))
        {
            NrPaired++;
        }

        DeviceNode* pKidBase = pRootNode->p1stKid;
        DeviceNode* pKid = pKidBase;

    KidLoop:
        while (pKid)
        {
            if (TryPairDeviceWithAnyDriver(pKid))
            {
                NrPaired++;
            }
            
            DeviceNode* pSib = pKid->pNxtSib;
            if (pSib)
            {
                NrPaired += LocalDevdrivPairAttempt(pSib);
                pSib = pSib->pNxtSib;
            }

            pKid = pKid->p1stKid;
        }
        if (pKidBase)
        {
            pKid = pKidBase->pNxtSib;
            pKidBase = pKid;
            goto KidLoop;
        }

        pRootNode = pRootNode->pNxtSib;
    }
    
    return NrPaired;
}
