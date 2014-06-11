package com.igware.dx_remote_agent;

import android.app.Application;

//The commented-out lines can be uncommented to enable crash reporting.

//import org.acra.*;
//import org.acra.annotation.*;

//@ReportsCrashes(formKey = "<key here>")

public class AppMain extends Application {
    @Override
    public void onCreate() {
        // The following line triggers the initialization of ACRA
//        ACRA.init(this);
        super.onCreate();
    }
}
