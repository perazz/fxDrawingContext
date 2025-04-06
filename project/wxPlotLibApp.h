/***************************************************************
 * Name:      wxChartsApp.h
 * Purpose:   Defines Application Class
 * Author:    Federico Perini (federico.perini@gmail.com)
 * Created:   2022-11-04
 * Copyright: Federico Perini ()
 * License:
 **************************************************************/

#ifndef WXCHARTSAPP_H
#define WXCHARTSAPP_H

#include <wx/app.h>
#include <art/artProvider.h>

class wxChartsApp : public wxApp
{
    public:
        virtual bool OnInit();
};

#endif // WXCHARTSAPP_H
