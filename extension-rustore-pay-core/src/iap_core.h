
#if defined(DM_PLATFORM_HTML5) || defined(DM_PLATFORM_ANDROID) || defined(DM_PLATFORM_IOS)

#ifndef IAPCORE_H
#define IAPCORE_H

struct IAPCore
{
    IAPCore()
    {
        memset(this, 0, sizeof(*this));
        m_authorizationStatus = false;
    }
    bool m_authorizationStatus;
};

#endif  // IAPCORE_H

#endif // DM_PLATFORM_HTML5 || DM_PLATFORM_ANDROID || DM_PLATFORM_IOS

