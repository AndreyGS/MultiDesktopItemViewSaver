#pragma once

#if !defined(AGSHELPERSAPI)
#define AGSHELPERSAPI __declspec(dllimport)
#endif

#define STATUS_ZERO_MONITORS_COUNT 0xE0000101

AGSHELPERSAPI int wchardecimaltoint(const WCHAR* input);
AGSHELPERSAPI inline bool operator==(RECT rt1, RECT rt2);

class DDRegistryExtension;

// This class is a singleton. It's instance obtains by DesktopDisplays::getDD()
// and when it no more need it's must be closed by DesktopDisplays::closeDD()
class AGSHELPERSAPI DesktopDisplays {
public:
    struct MonitorRects {
        RECT fullArea;
        RECT workArea;
    };

    enum class Corner {
        NORTH_EAST, SOUTH_EAST, SOUTH_WEST, NORTH_WEST 
    };

    DesktopDisplays(const DesktopDisplays&) = delete;
    DesktopDisplays(DesktopDisplays&&) = delete;
    DesktopDisplays& operator=(const DesktopDisplays&) = delete;
    DesktopDisplays& operator=(DesktopDisplays&&) = delete;

    inline virtual void refreshData();

    inline unsigned char getPrimaryMonitorIndex() const;
    inline bool getMonitorRects(unsigned char index, MonitorRects* mr) const;
    void getCornerMonitorRects(Corner corner, MonitorRects* mr) const;
    inline unsigned char getMonitorCount() const;
    inline RECT getFullDesktop() const;
    inline HWND getDesktopHandle() const;
    inline bool isValid() const;

    bool operator==(const DesktopDisplays& dd) const;

    static DesktopDisplays* getDD();
    static void closeDD();

    friend class ::DDRegistryExtension;
private:
    inline DesktopDisplays();

    // This construstor is workaround for DDRegistryExtension(int)
    // constructor, that supply empty DesktopDisplays instance to later
    inline DesktopDisplays(int);

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
    static int usage;
};
