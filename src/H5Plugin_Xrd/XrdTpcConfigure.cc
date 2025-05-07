#include "H5PluginHandle.hh"
#include <dlfcn.h>
#include <fcntl.h>

#include "XrdOuc/XrdOuca2x.hh"
#include "XrdOuc/XrdOucEnv.hh"
#include "XrdOuc/XrdOucStream.hh"
#include "XrdOuc/XrdOucPinPath.hh"
#include "XrdSfs/XrdSfsInterface.hh"



bool XrdHttpHandler::Configure(const char *configfn, XrdOucEnv *myEnv)
{
    XrdOucEnv cfgEnv;
    XrdOucStream Config(&m_log, getenv("XRDINSTANCE"), &cfgEnv, "=====> ");

    // m_log.setMsgMask(LogMask::Warning | LogMask::Error);

    // test if XrdEC is used
    usingEC = getenv("XRDCL_EC")? true : false;

    std::string authLib;
    std::string authLibParms;
    int cfgFD = open(configfn, O_RDONLY, 0);
    if (cfgFD < 0) {
        m_log.Emsg("Config", errno, "open config file", configfn);
        return false;
    }
    Config.Attach(cfgFD);
    static const char *cvec[] = { "*** http tpc plugin config:", 0 };
    Config.Capture(cvec);
    const char *val;
    while ((val = Config.GetMyFirstWord())) {
        if (!strcmp("http.desthttps", val)) {
            if (!(val = Config.GetWord())) {
                Config.Close();
                m_log.Emsg("Config", "http.desthttps value not specified");
                return false;
            }
            if (!strcmp("1", val) || !strcasecmp("yes", val) || !strcasecmp("true", val)) {
                m_desthttps = true;
            } else if (!strcmp("0", val) || !strcasecmp("no", val) || !strcasecmp("false", val)) {
                m_desthttps = false;
            } else {
                Config.Close();
                m_log.Emsg("Config", "https.desthttps value is invalid", val);
                return false;
            }
        } else if (!strcmp("tpc.timeout", val)) {
            if (!(val = Config.GetWord())) {
                m_log.Emsg("Config","tpc.timeout value not specified.");  return false;
            }
            if (XrdOuca2x::a2tm(m_log, "timeout value", val, &m_timeout, 0)) return false;
                // First byte timeout can be set separately from the continuous timeout.
            if ((val = Config.GetWord())) {
                if (XrdOuca2x::a2tm(m_log, "first byte timeout value", val, &m_first_timeout, 0)) return false;
            } else {
                m_first_timeout = 2*m_timeout;
            }
        }
    }
    Config.Close();

    // Internal override: allow xrdtpc to use a different ca dir from the one prepared by the xrootd
    // framework.  meant for exceptional situations where the site might need a specially-prepared set
    // of cas only for tpc (such as trying out various workarounds for libnss).  Explicitly disables
    // the NSS hack below.
    auto env_cadir = getenv("XRDTPC_CADIR");
    if (env_cadir) m_cadir = env_cadir;

    const char *cadir = nullptr, *cafile = nullptr;
    if ((cadir = env_cadir ? env_cadir : myEnv->Get("http.cadir"))) {
        m_cadir = cadir;
        if (!env_cadir) {
            m_ca_file.reset(new XrdTlsTempCA(&m_log, m_cadir));
            if (!m_ca_file->IsValid()) {
                m_log.Emsg("Config", "CAs / CRL generation for libcurl failed.");
                return false;
            }
        }
    }
    if ((cafile = myEnv->Get("http.cafile"))) {
        m_cafile = cafile;
    }
    if (!cadir && !cafile) {
        m_log.Emsg("Config", "neither xrd.tls cadir nor certfile value specified; is TLS enabled?");
        return false;
    }
    
    void *sfs_raw_ptr;
    if ((sfs_raw_ptr = myEnv->GetPtr("XrdSfsFileSystem*"))) {
        m_sfs = static_cast<XrdSfsFileSystem*>(sfs_raw_ptr);
        // m_log.Emsg("Config cafile:", cafile);
        // m_log.Emsg("Config XrdSfsFileSystem:", m_sfs);
        m_log.Emsg("Config", "Using filesystem object from the framework.");
        return true;
    } else {
        m_log.Emsg("Config", "No filesystem object available to HTTP-TPC subsystem.  Internal error.");
        return false;
    }
    
    return true;
}

