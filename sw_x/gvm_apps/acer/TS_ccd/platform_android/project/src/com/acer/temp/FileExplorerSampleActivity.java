package com.acer.temp;

import java.util.ArrayList;
import java.util.HashMap;

import com.acer.ccd.serviceclient.CcdiClient;
import com.acer.ccd.util.CcdSdkDefines.FileExplorer;

import android.app.Activity;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Log;

public class FileExplorerSampleActivity extends Activity {

    private final static String TAG = "FileExplorerSampleActivity";
    private CcdiClient mCcdiClient = null;
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mCcdiClient = new CcdiClient(getApplicationContext());
        mCcdiClient.onCreate();
    }
    @Override
    public void onStart() {
        super.onStart();
        mCcdiClient.onStart();
    }
    @Override
    public void onResume() {
        super.onResume();
        mCcdiClient.onResume();
        new BrowseFileTask().execute("1270028", "");
        new BrowseFileTask().execute("1270028", "photos");
    }

    @Override
    public void onPause() {
        super.onPause();
        mCcdiClient.onPause();
    }

    @Override
    public void onStop() {
        super.onStop();
        mCcdiClient.onStop();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        mCcdiClient.onDestroy();
    }

    private class BrowseFileTask extends AsyncTask<String, Void, Integer> {

        private ArrayList<HashMap<String, Object>> mResults = new ArrayList<HashMap<String, Object>>();

        @Override
        protected void onPostExecute(Integer code) {
            Log.i(TAG, "size: " + mResults.size());
            for (HashMap<String, Object> hm : mResults) {
                String name = (String) hm.get(FileExplorer.NAME);
                String fullpath = (String) hm.get(FileExplorer.FULLPATH);
                boolean isDir = (Boolean) hm.get(FileExplorer.ISDIR);
                Log.i(TAG, "name: " + name + ", fullpath: " + fullpath + ", isDir: " + isDir);
            }
        }

        @Override
        protected void onPreExecute() {
            super.onPreExecute();
            Log.v(TAG, "onPreExecute");
        }

        @Override
        protected Integer doInBackground(String... params) {
            Log.v(TAG, "doInBackground");
            int code = -1;
            code = mCcdiClient.doFileBrowse(Long.valueOf(params[0]), params[1], mResults);
            return Integer.valueOf(code);
        }
    }
}