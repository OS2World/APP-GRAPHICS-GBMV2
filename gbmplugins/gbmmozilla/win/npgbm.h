//********************************************************************************
// Mozilla GBM plug-in: npgbm.hpp
//
// Copyright (C) 2006-2012 Heiko Nitzsche
//
//   This software is provided 'as-is', without any express or implied
//   warranty.  In no event will the author be held liable for any damages
//   arising from the use of this software.
//
//   Permission is granted to anyone to use this software for any purpose,
//   including commercial applications, and to alter it and redistribute it
//   freely, subject to the following restrictions:
//
//   1. The origin of this software must not be misrepresented; you must not
//      claim that you wrote the original software. If you use this software
//      in a product, an acknowledgment in the product documentation would be
//      appreciated but is not required.
//   2. Altered source versions must be plainly marked as such, and must not be
//      misrepresented as being the original software.
//   3. This notice may not be removed or altered from any source distribution.

#ifndef __GBM_PLUGIN_H__
#define __GBM_PLUGIN_H__

#include "pluginbase.h"

#include "ConfigHandler.hpp"
#include "GbmDocument.hpp"
#include "GbmBitmapViewer.hpp"


class GbmPluginInstance : public nsPluginInstanceBase
{
    public:
        GbmPluginInstance(const nsPluginCreateData & createData);
        virtual ~GbmPluginInstance();

        // --- nsPluginInstanceBase interface ---
        
        virtual NPBool init(NPWindow* aWindow);
        virtual void shut();
        virtual NPBool isInitialized();

        virtual NPError SetWindow(NPWindow* pNPWindow);
        
        virtual NPError NewStream(NPMIMEType type, NPStream * stream, NPBool seekable, uint16 * stype);
        virtual NPError DestroyStream(NPStream * stream, NPError reason);
        virtual void StreamAsFile(NPStream * stream, const char * fname);
        virtual int32 WriteReady(NPStream * stream);
        virtual int32 Write(NPStream * stream, int32 offset, int32 len, void * buffer);
                              
        virtual void Print(NPPrint* printInfo);

    // --------------
    
    private:
        // Copy constructor is not implemented
        GbmPluginInstance(const GbmPluginInstance &); 
        // = operator is not implemented
        GbmPluginInstance & operator =(const GbmPluginInstance &);

        NPP    mInstance;
        NPBool mInitialized;
        
        NPWindow        * mpWindow;
        HWND              mHwnd;
        uint16            mMode;
        GbmDocument     * mpDocument;
        GbmBitmapViewer * mpViewer;
        int               mGbmTypeIndex;
        int               mProgress;
        int               mProgressMax;
};

#endif // __GBM_PLUGIN_H__
