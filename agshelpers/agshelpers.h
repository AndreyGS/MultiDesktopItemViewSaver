#pragma once

#if !defined(AGSHELPERSAPI)
#define AGSHELPERSAPI __declspec(dllimport)
#endif

#define STATUS_ZERO_MONITORS_COUNT 0xE0000101

AGSHELPERSAPI int wchardecimaltoint(const WCHAR* const input);
AGSHELPERSAPI inline bool operator!=(RECT rt1, RECT rt2);

class DDRegistryExtension;

class AGSHELPERSAPI DesktopDisplays {
public:
    struct MonitorRects {
        RECT fullArea;
        RECT workArea;
    };

    enum class Corner {
        NORTH_EAST, SOUTH_EAST, SOUTH_WEST, NORTH_WEST 
    };

    inline virtual void refreshData();

    inline unsigned char getPrimaryMonitorIndex() const;
    inline bool getMonitorRects(unsigned char index, MonitorRects* mr) const;
    void getCornerMonitorRects(Corner corner, MonitorRects* mr) const;
    inline unsigned char getMonitorCount() const;
    inline RECT getFullDesktop() const;
    inline HWND getDesktopHandle() const;

    inline bool isValid() const { return valid; }

    bool operator==(const DesktopDisplays& dd) const;

    static DesktopDisplays* getDD();
    static void closeDD();

    friend class ::DDRegistryExtension;
private:
    inline DesktopDisplays();
    inline ~DesktopDisplays();

    inline void cleanHeap();
    void initialize();
    void getDisplaysMetricsFromCurrentDesktop();
    inline void allocateMonitors();

    MonitorRects* monitors;
    RECT fulldesktop;
    unsigned char primmon;
    unsigned char moncount;

    bool valid;

    static DesktopDisplays* instance;
};
