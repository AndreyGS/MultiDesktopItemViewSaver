#pragma once

#include "../agshelpers/agshelpers.h"

class DDRegistryExtension
    : public DesktopDisplays
{
public:
    struct IndexRetriveResult {
        LSTATUS status;
        unsigned char index;
        unsigned char subkeysnum;
        bool keyexist;
    };

    DDRegistryExtension(int regnum);
    DDRegistryExtension(const DDRegistryExtension&) = delete;
    DDRegistryExtension& operator=(const DDRegistryExtension&) = delete;

    inline virtual void refreshData();

    void retriveCurrentProfileIndexFromRegistry();
    LSTATUS saveCurrentDesktopMetricsToRegistry() const;

    inline unsigned char getRegistryIndex() const { return index; }
    inline unsigned char getRegistrySubkeysNum() const { return subkeysnum; }

    // If key is not exist for now, value of index indicates
    // where potential saving to registry will be done
    inline bool isRegistryKeyExist() const { return keyexist; }

    static DDRegistryExtension* getDDRegExt();
    static void closeDDRegExt();
private:
    inline DDRegistryExtension();

    unsigned char index;
    unsigned char subkeysnum;
    bool keyexist;

    static DDRegistryExtension* instance;
};
