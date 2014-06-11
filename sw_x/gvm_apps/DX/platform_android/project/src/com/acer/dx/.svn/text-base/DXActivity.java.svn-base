package com.acer.dx;


import java.io.*;
import java.util.*;
import java.net.*;

import android.accounts.*;
import android.app.*;
import android.content.*;
import android.os.*;
import android.text.*;
import android.text.style.*;
import android.util.*;
import android.view.*;
import android.widget.*;
import android.provider.MediaStore;

import com.acer.dx.serviceclient.*;
import com.acer.dx.util.igware.Dataset;
import com.acer.dx.util.igware.Utils;

import igware.cloud.media_metadata.pb.MCAForControlPoint.GetMetadataInput;

import org.json.*;


public class DXActivity extends Activity {
    private static final String LOG_TAG = "DXActivity";
    private static final String NL = "\n";
    
    CcdiClient mCcdiClient = new CcdiClient(this);
    McaClient mMcaClient = new McaClient(this);
    Dataset[] mOwnedDatasets = null;
    SpannableStringBuilder builder = new SpannableStringBuilder();
    TextView mTextView;
    Button mTypeOfTestButton;
    View mLoggedInLoggedOutContainer;
    Button mLoggedInLoggedOutButton;
    Button mStartButton;
    Button mRunOnceButton;
    Button mStopButton;
    ProgressBar mProgressBar;
    boolean mContinuousTestRunning = false;
    boolean mShouldStop = false;
    private long mCameraRollDataset = -1;
    private boolean mFirstNLSkipped = false;
    private String mSWUpdateDownloadAppVersion = "";
    private boolean mCallCancel = false;
    
    private String mUsername = "dxuser@igware.com";
    private String mPassword = "password";
    
    private String mCustomGuid = null;
    private int mNumberOfTimesToStreamMedia = 1;
    
    private static String PREFS_NAME = "DXPrefs";
    
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        Log.i(LOG_TAG, "******** DXActivity onCreate " );

        super.onCreate(savedInstanceState);
        setContentView(R.layout.dx);
        mTextView = (TextView)findViewById(R.id.textView);
        mTypeOfTestButton = (Button)findViewById(R.id.TypeOfTest);
        mLoggedInLoggedOutContainer = (View)findViewById(R.id.loginLogoutContainer);
        mLoggedInLoggedOutButton = (Button)findViewById(R.id.loginLogout);
        mStartButton = (Button)findViewById(R.id.Start);
        mRunOnceButton = (Button)findViewById(R.id.Run_Once);
        mStopButton = (Button)findViewById(R.id.Stop);
        mProgressBar = (ProgressBar)findViewById(R.id.progressBar);
        
        setInitialState();
        
        Log.i(LOG_TAG, "******** about to create mCcdiClient " );
        mCcdiClient.onCreate();
        Log.i(LOG_TAG, "******** created mCcdiClient " );
        
        SharedPreferences settings = getSharedPreferences(PREFS_NAME, MODE_PRIVATE);
        mUsername = settings.getString("username", mUsername);
        mPassword = settings.getString("password", mPassword);
        mNumberOfTimesToStreamMedia = settings.getInt("numberOfTimesToStreamMedia", mNumberOfTimesToStreamMedia);
    }

    @Override
    protected void onStart() {
        Log.i(LOG_TAG, "******** DXActivity onStart " );
        
        super.onStart();

        Log.i(LOG_TAG, "******** about to call mCcdiClient.onStart " );
        mCcdiClient.onStart();
        Log.i(LOG_TAG, "******** called mCcdiClient.onStart " );
        
        Log.i(LOG_TAG, "******** about to call mMcaClient.onStart " );
        mMcaClient.onStart();
        Log.i(LOG_TAG, "******** called mMcaClient.onStart " );
        
    }

    @Override
    protected void onStop() {
        Log.i(LOG_TAG, "******** DXActivity onStop " );

        super.onStop();

        Log.i(LOG_TAG, "******** about to call mCcdiClient.onStop " );
        mCcdiClient.onStop();
        Log.i(LOG_TAG, "******** called mCcdiClient.onStop " );
        
        Log.i(LOG_TAG, "******** about to call mMcaClient.onStop " );
        mMcaClient.onStop();
        Log.i(LOG_TAG, "******** called mMcaClient.onStop " );
    }
    
    public void onClick_TypeOfTest(View view) {
        ArrayList<CharSequence> itemsList = new ArrayList<CharSequence>();
        itemsList.add(getString(R.string.Login_Sequence));
        itemsList.add(getString(R.string.Upload_Download));
        itemsList.add(getString(R.string.Media_Client));
        itemsList.add(getString(R.string.NumberOfTimesToStreamMedia));
        itemsList.add(getString(R.string.Sw_Update));
        itemsList.add(getString(R.string.Download_Acer_Apps));
        itemsList.add(getString(R.string.DownloadCustomGuid));
        itemsList.add(getString(R.string.changeUserNamePassword));

        CharSequence text = mTextView.getText();
        if(mRunOnceButton.isEnabled() && (text != null) && (text.length() > 0) )
        itemsList.add(getString(R.string.uploadLog));
        
        final CharSequence[] items = itemsList.toArray(new CharSequence[itemsList.size()]);

        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle(R.string.pick_a_test);
        builder.setItems(items, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int item) {
                if(items[item].equals(getString(R.string.uploadLog)))
                    showDescriptionDialog();
                else if(items[item].equals(getString(R.string.changeUserNamePassword)))
                    showUsernamePasswordDialog();
                else if(items[item].equals(getString(R.string.NumberOfTimesToStreamMedia)))
                    showNumberOfTimesToStreamMediaDialog();
                else
                {
                    mTypeOfTestButton.setText(items[item]);
                    if(isSWUpdateTestChosen())
                        mLoggedInLoggedOutContainer.setVisibility(View.VISIBLE);
                    else
                        mLoggedInLoggedOutContainer.setVisibility(View.GONE);
                    
                    if(items[item].equals(getString(R.string.DownloadCustomGuid)))
                        showCustomGuidDialog();
                }
            }
        });
        AlertDialog alert = builder.create();
        alert.show();
    }
    
    public void onClick_PickLoginLogout(View view) {
        ArrayList<CharSequence> itemsList = new ArrayList<CharSequence>();
        itemsList.add(getString(R.string.runLoggedIn));
        itemsList.add(getString(R.string.runLoggedOut));

        final CharSequence[] items = itemsList.toArray(new CharSequence[itemsList.size()]);

        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle(R.string.pick_a_test);
        builder.setItems(items, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int item) {
                mLoggedInLoggedOutButton.setText(items[item]);
            }
        });
        AlertDialog alert = builder.create();
        alert.show();
    }
    
    private boolean isSWUpdateTestChosen()
    {
        return getString(R.string.Sw_Update).equals(mTypeOfTestButton.getText()) ||
                getString(R.string.Download_Acer_Apps).equals(mTypeOfTestButton.getText()) ||
                getString(R.string.DownloadCustomGuid).equals(mTypeOfTestButton.getText());
    }
    
    private void clearState()
    {
        mTextView.setText("");
        if(builder.length() > 0 )
            builder = new SpannableStringBuilder();
    }
    
    public void onClick_Start(View view) {
        
        clearState();
     
        mTypeOfTestButton.setEnabled(false);
        mLoggedInLoggedOutButton.setEnabled(false);
        mStartButton.setEnabled(false);
        mRunOnceButton.setEnabled(false);
        mStopButton.setEnabled(true);
        
        mContinuousTestRunning = true;
        mShouldStop = false;
        mProgressBar.setVisibility(View.VISIBLE);
        
        startAppropriateTask();
    }
    
    private void startAppropriateTask()
    {
        if( isSWUpdateTestChosen() && getString(R.string.runLoggedOut).equals(mLoggedInLoggedOutButton.getText()) )
            new SetCcdVersionTask().execute();
        else
            new DoLoginTask().execute();
    }
    
    public void onClick_Run_Once(View view) {
        
        clearState();
        
        mTypeOfTestButton.setEnabled(false);
        mLoggedInLoggedOutButton.setEnabled(false);
        mStartButton.setEnabled(false);
        mRunOnceButton.setEnabled(false);
        mStopButton.setEnabled(false);
        
        mContinuousTestRunning = false;
        mShouldStop = false;
        mProgressBar.setVisibility(View.VISIBLE);
        
        startAppropriateTask();
    }
  
    public void onClick_Stop(View view) {
        
        mTypeOfTestButton.setEnabled(false);
        mLoggedInLoggedOutButton.setEnabled(false);
        mStartButton.setEnabled(false);
        mRunOnceButton.setEnabled(false);
        mStopButton.setEnabled(false);
        
        mShouldStop = true;
    }
    
    private static final String ACCOUNT_TYPE = "com.acer.accounts.ACERCLOUD";
    private class DoLoginTask extends AsyncTask<Void, Void, Void> {
        
        private int result = -1;

        @Override
        protected Void doInBackground(Void... params) {
            Log.i(LOG_TAG, "********** DoLoginTask doInBackground");
            
            long time1 = System.currentTimeMillis();
            
            Account[] accounts = AccountManager.get(DXActivity.this).getAccountsByType(ACCOUNT_TYPE);
            if(accounts.length > 0) //another user is logged in through ccd and not dxuser
            {
                result = -2;
                return null;
            }
            
//          result = mCcdiClient.doLogin("dxuser77@gmail.com", "dxPassword");
            result = mCcdiClient.doLogin(mUsername, mPassword);

            long time2 = System.currentTimeMillis();
            String logString = "******** login result: " + result + " using username/password: ##" + mUsername + "/" + mPassword + "##";
            if(result != 0)
                logString = "******** login result: @@" + result + "@@ using username/password: ##" + mUsername + "/" + mPassword + "##";

            if( ! mFirstNLSkipped)
                mFirstNLSkipped = true;
            else
                builder.append(NL);
            builder.append(formatDisplayText(logString));
            builder.append(NL);
            builder.append("---- login took : " + (time2 - time1) + " ms");
            builder.append(NL);
            
            Log.i(LOG_TAG, logString);
            
            return null;
        }
        
        protected void onPostExecute(Void unused) {
            mTextView.setText(builder);
            
            if(result == 0)
                new LinkDeviceTask().execute();
            else if(result == -2)
                Toast.makeText(DXActivity.this, R.string.removeAcerAccount, Toast.LENGTH_LONG).show();
            
            if(result != 0)
                handleNextState();
        }

    }
    
    private class LinkDeviceTask extends AsyncTask<Void, Void, Void> {

        private int result = -1;
        @Override
        protected Void doInBackground(Void... params) {
            Log.i(LOG_TAG, "********** LinkDeviceTask doInBackground");
            
            long time1 = System.currentTimeMillis();
            result = mCcdiClient.linkDevice("DX Test Device");
            long time2 = System.currentTimeMillis();

            String logString = "******** linkDevice result: " + result;
            if(result != 0)
                logString = "******** linkDevice result: @@" + result + "@@";

            
//            builder.append(NL);
            builder.append(formatDisplayText(logString));
            builder.append(NL);
            builder.append("---- linkDevice took : " + (time2 - time1) + " ms");
            builder.append(NL);
            
            Log.i(LOG_TAG, logString);
            
            return null;
        }
        
        protected void onPostExecute(Void unused) {
            mTextView.setText(builder);
            
            if(result == 0)
            {
                if(isSWUpdateTestChosen())
                    new SetCcdVersionTask().execute();
                else
                    new ListOwnedDatasetsTask().execute();
            }
            else
                new DoLogoutTask().execute();
        }
    }
    
    private class SetCcdVersionTask extends AsyncTask<Void, Void, Void> {

        @Override
        protected Void doInBackground(Void... params) {
            Log.i(LOG_TAG, "********** SetCcdVersionTask doInBackground");

            long time1 = System.currentTimeMillis();
            int result = mCcdiClient.swUpdateSetCcdVersion("test-dxand-swu-ccd", "1.1.2.1");
            long time2 = System.currentTimeMillis();
            
            String logString = "******** SetCcdVersionTask result: " + result ;
            if(result != 0)
                logString = "******** SetCcdVersionTask result: @@" + result + "@@";

//            builder.append(NL);
            builder.append(formatDisplayText(logString));
            builder.append(NL);
            builder.append("---- SetCcdVersionTask took : " + (time2 - time1) + " ms");
            builder.append(NL);

            Log.i(LOG_TAG, logString);
           
            return null;
        }
        
        protected void onPostExecute(Void unused) {
            mTextView.setText(builder);
            
            if(getString(R.string.Sw_Update).equals(mTypeOfTestButton.getText()))
                new SWUpdateCheckTask().execute();
            else if ( (getString(R.string.DownloadCustomGuid).equals(mTypeOfTestButton.getText())) &&
                    Utils.isStringNotEmpty(mCustomGuid))
                new DownloadCustomAppPolledTask().execute();
            else if(getString(R.string.Download_Acer_Apps).equals(mTypeOfTestButton.getText()))
                new DownloadAcerAppsPolledTask(0).execute();
        }
    }
    
    private class DownloadCustomAppPolledTask extends AsyncTask<Void, Void, Void> {
        @Override
        protected Void doInBackground(Void... params) {
            Log.i(LOG_TAG, "********** DownloadCustomAppTask doInBackground mCustomGuid: " + mCustomGuid);

            int result = doAppDownload(mCustomGuid, mCustomGuid + ".apk", true);

            Log.i(LOG_TAG, "******** doAppDownload result: " + result);
           
            return null;
        }
        
        protected void onPostExecute(Void unused) {
            mTextView.setText(builder);
            
            Log.i(LOG_TAG, "********** DownloadCustomAppTask onPostExecute mCustomGuid: " + mCustomGuid);

            new DownloadCustomAppEventDrivenTask().execute();
        }
    }
    
    private class DownloadCustomAppEventDrivenTask extends AsyncTask<Void, Void, Void> {
        @Override
        protected Void doInBackground(Void... params) {
            Log.i(LOG_TAG, "********** DownloadCustomAppTask doInBackground mCustomGuid: " + mCustomGuid);

            int result = doAppDownload(mCustomGuid, mCustomGuid + ".apk", false);

            Log.i(LOG_TAG, "******** doAppDownload result: " + result);
           
            return null;
        }
        
        protected void onPostExecute(Void unused) {
            mTextView.setText(builder);
            
            Log.i(LOG_TAG, "********** DownloadCustomAppTask onPostExecute mCustomGuid: " + mCustomGuid);

            if (mCcdiClient.isLoggedIn())
                new DoUnlinkTask().execute();
            else
            {
                builder.append(NL);
                handleNextState();
            }
        }
    }
    
    private static String[] ACER_APP_GUIDS = new String[] {
        "951D885F-BD9F-4F74-A013-E296B194F46B",
        "765C60BE-95A4-46C7-AB56-38104B4D438A",
        "D81B5307-CCFE-404F-BEA5-2D9C7B3894AA",
        "0CA4401A-BAC6-4280-932D-EBDCC89C5BB0",
        "74E844FA-2038-48B7-A9A4-61936AF0149D"};
            
    private static String[] ACER_APP_NAMES = new String[] {
        "AcerCloud.apk",
        "clear.fiPhoto.apk",
        "clear.fiVideo.apk",
        "clear.fiMusic.apk",
        "AcerCloudDocs.apk"};
    
    private class DownloadAcerAppsPolledTask extends AsyncTask<Void, Void, Void> {
        
        private int mAppIndex;
        
        public DownloadAcerAppsPolledTask(int index)
        {
            mAppIndex = index;
        }
        
        @Override
        protected Void doInBackground(Void... params) {
            Log.i(LOG_TAG, "********** DownloadAcerAppsTask doInBackground mAppIndex: " + mAppIndex);

            int result = doAppDownload(ACER_APP_GUIDS[mAppIndex], ACER_APP_NAMES[mAppIndex], true);

            Log.i(LOG_TAG, "******** doAppDownload result: " + result);
           
            return null;
        }
        
        protected void onPostExecute(Void unused) {
            mTextView.setText(builder);
            new DownloadAcerAppsEventDrivenTask(mAppIndex).execute();
        }
    }

    private class DownloadAcerAppsEventDrivenTask extends AsyncTask<Void, Void, Void> {
        
        private int mAppIndex;
        
        public DownloadAcerAppsEventDrivenTask(int index)
        {
            mAppIndex = index;
        }
        
        @Override
        protected Void doInBackground(Void... params) {
            Log.i(LOG_TAG, "********** DownloadAcerAppsTask doInBackground mAppIndex: " + mAppIndex);

            int result = doAppDownload(ACER_APP_GUIDS[mAppIndex], ACER_APP_NAMES[mAppIndex], false);

            Log.i(LOG_TAG, "******** doAppDownload result: " + result);
           
            return null;
        }
        
        protected void onPostExecute(Void unused) {
            mTextView.setText(builder);
            
            mAppIndex++;
            Log.i(LOG_TAG, "********** DownloadAcerAppsTask onPostExecute mAppIndex: " + mAppIndex);

            if(mAppIndex < ACER_APP_GUIDS.length)
                new DownloadAcerAppsPolledTask(mAppIndex).execute();
            else if (mCcdiClient.isLoggedIn())
                new DoUnlinkTask().execute();
            else
            {
                builder.append(NL);
                handleNextState();
            }
        }
    }
    
    private int doAppDownload(String guid, String appName, boolean doPolled)
    {
        long time1 = System.currentTimeMillis();
        JSONObject jsonObject = new JSONObject();
        
        if(doPolled)
            appName = "polled-" + appName;
        else
            appName = "eventDriven-" + appName;

        int result = mCcdiClient.swUpdateCheck(guid, "0.0.0.0", jsonObject);
        long time2 = System.currentTimeMillis();
        
        String logString = "******** SWUpdateCheckTask result: " + result + " for " + guid;
        if(result != 0)
            logString = "******** SWUpdateCheckTask result: @@" + result + "@@ for ##" + guid + "##";
        builder.append(formatDisplayText(logString));
        builder.append(NL);
       
        try
        {
            String version = jsonObject.getString("LatestAppVersion");
            logString = "******** SWUpdateCheckTask UpdateMask: " + jsonObject.getString("UpdateMask") + "\n" +
                "******** SWUpdateCheckTask LatestAppVersion: " + jsonObject.getString("LatestAppVersion") + "\n" +
                "******** SWUpdateCheckTask LatestCcdVersion: " + jsonObject.getString("LatestCcdVersion") + "\n" +
                "******** SWUpdateCheckTask ChangeLog: " + jsonObject.getString("ChangeLog");
            
            builder.append(formatDisplayText(logString));
            builder.append(NL);
            builder.append("---- SWUpdateCheckTask took : " + (time2 - time1) + " ms");
            builder.append(NL);
            Log.i(LOG_TAG, logString);
            
            jsonObject = new JSONObject();

            time1 = System.currentTimeMillis();
            long handle;
            if(doPolled)
            {
                handle = mCcdiClient.doPolledSwUpdateDownload(guid, version, jsonObject, 400, false);
                result = jsonObject.getInt("result");
            }
            else
            {
                handle = mCcdiClient.doEventDrivenSwUpdateDownload(mCcdiClient.eventsCreateQueue(), guid, version, false);
                mCcdiClient.destroyQueue(handle);
                if( handle > 0 )
                    result = 0;
                else
                    result = -1;
            }
            time2 = System.currentTimeMillis();

            if(doPolled)
                logString = "******** POLLED doAppDownload result: " + result;
            else
                logString = "******** EVENT DRIVEN doAppDownload result: " + result ;
                
            if(result != 0)
            {
                if(doPolled)
                    logString = "******** POLLED doAppDownload result: @@" + result + "@@";
                else
                    logString = "******** EVENT DRIVEN doAppDownload result: @@" + result + "@@";
            }
            
            builder.append(formatDisplayText(logString));
            builder.append(NL);
            if(doPolled)
                builder.append("---- POLLED doAppDownload took : " + (time2 - time1) + " ms");
            else
                builder.append("---- EVENT DRIVEN doAppDownload took : " + (time2 - time1) + " ms");

            builder.append(NL);
            Log.i(LOG_TAG, logString);
            
            jsonObject = new JSONObject();

            time1 = System.currentTimeMillis();
            result = mCcdiClient.swUpdateEndDownload(handle, CcdiClient.TEMP_SD_CARD_SW_PATH + appName);
            time2 = System.currentTimeMillis();
            
            logString = "******** EndDownloadSWUpdateTask result: " + result + " file written: " + appName;
            if(result != 0)
                logString = "******** EndDownloadSWUpdateTask result: @@" + result + "@@" + " file written: " + appName;
            
//            builder.append(NL);
            builder.append(formatDisplayText(logString));
            builder.append(NL);
            builder.append("---- EndDownloadSWUpdateTask took : " + (time2 - time1) + " ms");
            builder.append(NL);

            Log.i(LOG_TAG, logString);
            
            return result;
        }
        catch(org.json.JSONException e )
        {
            builder.append(formatDisplayText("@@ERROR: " + e.getMessage() + "@@"));
            builder.append(NL);
            Utils.logException(e);
            return -1;
        }
    }
    
    private class SWUpdateCheckTask extends AsyncTask<Void, Void, Void> {
        
        @Override
        protected Void doInBackground(Void... params) {
            Log.i(LOG_TAG, "********** SWUpdateCheckTask doInBackground");

            long time1 = System.currentTimeMillis();
            JSONObject jsonObject = new JSONObject();

            int result = mCcdiClient.swUpdateCheck("test-dxand-swu-001", "1.1.1.0", jsonObject);
            long time2 = System.currentTimeMillis();
            
            String logString = "******** SWUpdateCheckTask result: " + result;
            if(result != 0)
                logString = "******** SWUpdateCheckTask result: @@" + result + "@@";
            builder.append(formatDisplayText(logString));
            builder.append(NL);
            
            try
            {
                mSWUpdateDownloadAppVersion = jsonObject.getString("LatestAppVersion");
                logString = "******** SWUpdateCheckTask UpdateMask: " + jsonObject.getString("UpdateMask") + "\n" +
                    "******** SWUpdateCheckTask LatestAppVersion: " + jsonObject.getString("LatestAppVersion") + "\n" +
                    "******** SWUpdateCheckTask LatestCcdVersion: " + jsonObject.getString("LatestCcdVersion") + "\n" +
                    "******** SWUpdateCheckTask ChangeLog: " + jsonObject.getString("ChangeLog");
                builder.append(formatDisplayText(logString));
                builder.append(NL);
            }
            catch(org.json.JSONException e )
            {
                builder.append(formatDisplayText("@@ERROR: " + e.getMessage() + "@@"));
                builder.append(NL);
                Utils.logException(e);
            }
            
            builder.append("---- SWUpdateCheckTask took : " + (time2 - time1) + " ms");
            builder.append(NL);

            Log.i(LOG_TAG, logString);
           
            return null;
        }
        
        protected void onPostExecute(Void unused) {
            mTextView.setText(builder);
            
            mCallCancel = false;
            
            new PolledDownloadSWUpdateTask().execute();
        }
    }
    
    private class PolledDownloadSWUpdateTask extends AsyncTask<Void, Void, Void> {
        
        private long mHandle = -1;
        
        @Override
        protected Void doInBackground(Void... params) {
            Log.i(LOG_TAG, "********** PolledDownloadSWUpdateTask doInBackground");
            
            JSONObject jsonObject = new JSONObject();

            long time1 = System.currentTimeMillis();
            mHandle = mCcdiClient.doPolledSwUpdateDownload("test-dxand-swu-001", mSWUpdateDownloadAppVersion, jsonObject, 1000, mCallCancel);
            long time2 = System.currentTimeMillis();
            
            boolean isLoggedIn = mCcdiClient.isLoggedIn();
            String logString;
            if(isLoggedIn)
                logString = "******** LOGGED IN POLLED DownloadSWUpdateTask result: ";
            else            
                logString = "******** LOGGED OUT POLLED DownloadSWUpdateTask result: ";
            
            try{
                if(jsonObject.getInt("result") == 0)
                    logString += jsonObject.getInt("result") ;
                else
                    logString += "@@" + jsonObject.getInt("result") + "@@";
            }
            catch(org.json.JSONException e )
            {
                Utils.logException(e);
                logString += "undefined";
            }
            
            logString += " mCallCancel: " + mCallCancel;

//            builder.append(NL);
            builder.append(formatDisplayText(logString));
            builder.append(NL);
            if(isLoggedIn)
                builder.append("---- LOGGED IN POLLED DownloadSWUpdateTask took : " + (time2 - time1) + " ms");
            else    
                builder.append("---- LOGGED OUT POLLED DownloadSWUpdateTask took : " + (time2 - time1) + " ms");
            builder.append(NL);

            Log.i(LOG_TAG, logString);
           
            return null;
        }
        
        protected void onPostExecute(Void unused) {
            mTextView.setText(builder);
            
            new EndPolledDownloadSWUpdateTask(mHandle).execute();
        }
    }
    
    private class EndPolledDownloadSWUpdateTask extends AsyncTask<Void, Void, Void> {
        
        private long mHandle = -1;
        
        public EndPolledDownloadSWUpdateTask(long handle)
        {
            mHandle = handle;
        }

        @Override
        protected Void doInBackground(Void... params) {
            Log.i(LOG_TAG, "********** EndPolledDownloadSWUpdateTask doInBackground");
            
            boolean isLoggedIn = mCcdiClient.isLoggedIn();
            String title = CcdiClient.TEMP_SD_CARD_SW_FILE + "Polled";
            if(mCcdiClient.isLoggedIn())
                title += "LoggedIn";
            else
                title += "LoggedOut";
            
            if(mCallCancel)
                title += "Cancelled";
            
            long time1 = System.currentTimeMillis();
            int result = mCcdiClient.swUpdateEndDownload(mHandle, title);
            long time2 = System.currentTimeMillis();
            
            String logString = "******** EndPolledDownloadSWUpdateTask result: " + result + " file written: " + title;
            if(result != 0)
                logString = "******** EndPolledDownloadSWUpdateTask result: @@" + result + "@@" + " file written: " + title;
            
//            builder.append(NL);
            builder.append(formatDisplayText(logString));
            builder.append(NL);
            builder.append("---- EndPolledDownloadSWUpdateTask took : " + (time2 - time1) + " ms");
            builder.append(NL);

            Log.i(LOG_TAG, logString);
           
            return null;
        }
        
        protected void onPostExecute(Void unused) {
            mTextView.setText(builder);

            new EventsCreateQueueTask().execute();
        }
    }
    
    private class EventsCreateQueueTask extends AsyncTask<Void, Void, Void> {
        
        private long mqueueHandle;
        
        @Override
        protected Void doInBackground(Void... params) {
            Log.i(LOG_TAG, "********** EventsCreateQueueTask doInBackground");

            long time1 = System.currentTimeMillis();
            mqueueHandle = mCcdiClient.eventsCreateQueue();
            long time2 = System.currentTimeMillis();
            
            String logString = "******** EventsCreateQueueTask queue handle: " + mqueueHandle + " mCallCancel: " + mCallCancel;

//            builder.append(NL);
            builder.append(formatDisplayText(logString));
            builder.append(NL);
            builder.append("---- EventsCreateQueueTask took : " + (time2 - time1) + " ms");
            builder.append(NL);

            Log.i(LOG_TAG, logString);
           
            return null;
        }
        
        protected void onPostExecute(Void unused) {
            mTextView.setText(builder);
            
            new EventDrivenDownloadSWUpdateTask(mqueueHandle).execute();
        }
    }
    
    private class EventDrivenDownloadSWUpdateTask extends AsyncTask<Void, Void, Void> {
        
        private long mHandle = -1;
        private long mqueueHandle;
        
        public EventDrivenDownloadSWUpdateTask(long queueHandle)
        {
            mqueueHandle = queueHandle;
        }
        
        @Override
        protected Void doInBackground(Void... params) {
            Log.i(LOG_TAG, "********** EventDrivenDownloadSWUpdateTask doInBackground mCallCancel: " + mCallCancel);
            
            JSONObject jsonObject = new JSONObject();

            long time1 = System.currentTimeMillis();
            mHandle = mCcdiClient.doEventDrivenSwUpdateDownload(mqueueHandle, "test-dxand-swu-001", mSWUpdateDownloadAppVersion, mCallCancel);
            long time2 = System.currentTimeMillis();
            
            boolean isLoggedIn = mCcdiClient.isLoggedIn();
            String logString;
            if(isLoggedIn)
                logString = "******** LOGGED IN EVENT DRIVEN DownloadSWUpdateTask handle: " + mHandle;
            else 
                logString = "******** LOGGED OUT EVENT DRIVEN DownloadSWUpdateTask handle: " + mHandle;

            builder.append(formatDisplayText(logString));
            builder.append(NL);
            if(isLoggedIn)
                builder.append("---- LOGGED IN EVENT DRIVEN DownloadSWUpdateTask took : " + (time2 - time1) + " ms");
            else
                builder.append("---- LOGGED OUT EVENT DRIVEN DownloadSWUpdateTask took : " + (time2 - time1) + " ms");
            builder.append(NL);

            Log.i(LOG_TAG, logString);
            
            time1 = System.currentTimeMillis();
            int result =  mCcdiClient.destroyQueue(mqueueHandle);
            time2 = System.currentTimeMillis();
            Log.i(LOG_TAG, "********** EventDrivenDownloadSWUpdateTask destroy queue: " + result );
            builder.append("---- destroyQueue took : " + (time2 - time1) + " ms");
            builder.append(NL);
            
            return null;
        }
        
        protected void onPostExecute(Void unused) {
            mTextView.setText(builder);
            
            new EndEventDrivenDownloadSWUpdateTask(mHandle).execute();
        }
    }
    

    private class EndEventDrivenDownloadSWUpdateTask extends AsyncTask<Void, Void, Void> {
        
        private long mHandle = -1;
        boolean mIsLoggedIn;
        
        public EndEventDrivenDownloadSWUpdateTask(long handle)
        {
            mHandle = handle;
        }

        @Override
        protected Void doInBackground(Void... params) {
            Log.i(LOG_TAG, "********** EndEventDrivenDownloadSWUpdateTask doInBackground");
            
            mIsLoggedIn = mCcdiClient.isLoggedIn();
            String title = CcdiClient.TEMP_SD_CARD_SW_FILE + "EventDriven";
            if(mIsLoggedIn)
                title += "LoggedIn";
            else
                title += "LoggedOut";
            
            if(mCallCancel)
                title += "Cancelled";
            
            long time1 = System.currentTimeMillis();
            int result = mCcdiClient.swUpdateEndDownload(mHandle, title);
            long time2 = System.currentTimeMillis();
            
            String logString = "******** EndEventDrivenDownloadSWUpdateTask result: " + result + " file written: " + title;
            if(result != 0)
                logString = "******** EndEventDrivenDownloadSWUpdateTask result: @@" + result + "@@" + " file written: " + title;
            
//            builder.append(NL);
            builder.append(formatDisplayText(logString));
            builder.append(NL);
            builder.append("---- EndEventDrivenDownloadSWUpdateTask took : " + (time2 - time1) + " ms");
            builder.append(NL);

            Log.i(LOG_TAG, logString);
           
            return null;
        }
        
        protected void onPostExecute(Void unused) {
            mTextView.setText(builder);

            if( ! mCallCancel)
            {
                mCallCancel = true;
                new PolledDownloadSWUpdateTask().execute();
            }
            else if (mIsLoggedIn)
                new DoUnlinkTask().execute();
            else 
            {
                builder.append(NL);
                handleNextState();
            }
        }
    }
    
    private class ListOwnedDatasetsTask extends AsyncTask<Void, Void, Void> {

        @Override
        protected Void doInBackground(Void... params) {
            Log.i(LOG_TAG, "********** ListOwnedDatasetsTask doInBackground");

            long time1 = System.currentTimeMillis();
            mOwnedDatasets = mCcdiClient.listOwnedDataSets();
            long time2 = System.currentTimeMillis();
            
            String logString = "******** listOwnedDataSets result: " + mOwnedDatasets + " length: " + mOwnedDatasets.length;
            if(mOwnedDatasets.length == 0)
                logString = "******** listOwnedDataSets result: @@" + mOwnedDatasets + "@@ length: @@" + mOwnedDatasets.length + "@@";
            
//            builder.append(NL);
            builder.append(formatDisplayText(logString));
            builder.append(NL);
            builder.append("---- listOwnedDataSets took : " + (time2 - time1) + " ms");
            builder.append(NL);

            Log.i(LOG_TAG, logString);
           
            return null;
        }
        
        protected void onPostExecute(Void unused) {
            mTextView.setText(builder);
            
            mCameraRollDataset = -1;
            
            if( (mOwnedDatasets != null) && (mOwnedDatasets.length > 0) )
                new AddSubscriptionsTask(0).execute();
            else
                new DoUnlinkTask().execute();
        }
    }
    
    private class AddSubscriptionsTask extends AsyncTask<Void, Void, Void> {

        private int mIndex = -1;
        
        public AddSubscriptionsTask(int index)
        {
            mIndex = index;
        }
        
        @Override
        protected Void doInBackground(Void... params) {
            Log.i(LOG_TAG, "********** AddSubscriptionsTask doInBackground");

            if( (mOwnedDatasets == null) || (mOwnedDatasets.length == 0) )
                return null;
            
            if( (mIndex < 0) || (mIndex >= mOwnedDatasets.length ) )
                return null;
            
            int subscribeResult;
            
            Dataset dataset = mOwnedDatasets[mIndex];
            if(dataset == null)
                return null;
            
            if (dataset.getName().equals("CR Upload"))
                mCameraRollDataset = dataset.getDatasetId();
            
            String logString = "********** about to subscribe to dataset: " + dataset + " dataset id: " + dataset.getDatasetId();
            Log.i(LOG_TAG, logString);
            
            builder.append(formatDisplayText(logString));
            builder.append(NL);
            
            long time1 = System.currentTimeMillis();
            if(dataset.getName().equals("CR Upload"))
                subscribeResult = mCcdiClient.addCameraSyncUploadSubscription();
            else if (dataset.getName().equals("CameraRoll"))
                subscribeResult = mCcdiClient.addCameraSyncDownloadSubscription(0,0);
            else
                subscribeResult = mCcdiClient.subscribeDataset(dataset);
            long time2 = System.currentTimeMillis();

            logString = "---- subscribeDataset result: " + subscribeResult;
            if(subscribeResult != 0)
                logString = "---- subscribeDataset result: @@" + subscribeResult + "@@";

//                builder.append(NL);
            builder.append(formatDisplayText(logString));
            builder.append(NL);
            builder.append("---- subscribeDataset took : " + (time2 - time1) + " ms");
            builder.append(NL);

            Log.i(LOG_TAG, logString);
            
            return null;
        }
        
        protected void onPostExecute(Void unused) {
            
            mTextView.setText(builder);
            
            if(mIndex < mOwnedDatasets.length)
                new AddSubscriptionsTask(++mIndex).execute();
            else if( getString(R.string.Upload_Download).equals(mTypeOfTestButton.getText()) && (mCameraRollDataset != -1) )
                new UploadTask(1).execute();
            else if( getString(R.string.Media_Client).equals(mTypeOfTestButton.getText()) )
                new MediaClientTask().execute();
            else
                new DoUnlinkTask().execute();
        }
    }

    private static int NUM_TIMES_TO_UPLOAD_DOWNLOAD = 5;
    private class UploadTask extends AsyncTask<Void, Void, Void> {
      
        private int count; 
        private boolean shouldCallHandleNextState = false;
        
        public UploadTask(int c) 
        {
            count = c;
        }
        
        @Override
        protected Void doInBackground(Void... params) {
            Log.i(LOG_TAG, "********** UploadTask doInBackground");
            Log.i(LOG_TAG, "********** UploadTask count: " + count);
            
            if (mCameraRollDataset >=  0) {
                try{
                    long time1 = System.currentTimeMillis();
                    final InputStream is = getResources().getAssets().open("test.jpg");
                    String fileName = "test" + count + ".jpg";
                    int result = mCcdiClient.doFileUpload(fileName, mCameraRollDataset, is);
                    long time2 = System.currentTimeMillis();
                    
                    String logString = "******** doFileUpload for " + fileName + " result: " + result;
                    if(result != 0)
                        logString = "******** doFileUpload for @@" + fileName + "@@ result: @@" + result + "@@";
        
                    builder.append(formatDisplayText(logString));
                    builder.append(NL);
                    builder.append("---- doFileUpload took : " + (time2 - time1) + " ms");
                    builder.append(NL);

                    Log.i(LOG_TAG, logString);
                }
                catch (IOException ioe) {
                    Utils.logException(ioe);
                }
            }
            
            return null;
        }
        
        protected void onPostExecute(Void unused) {
            mTextView.setText(builder);
            
            if(count < NUM_TIMES_TO_UPLOAD_DOWNLOAD)
                new UploadTask(++count).execute();
            else
                handleNextState();
        }
    }
    
//    private class DownloadTask extends AsyncTask<Void, Void, Void> {
//        
//        private long myCloudDatasetId = -1;
//        private int count; 
//        
//        public DownloadTask(long id, int c) 
//        {
//            myCloudDatasetId = id;
//            count = c;
//        }
//        
//        @Override
//        protected Void doInBackground(Void... params) {
//            Log.i(LOG_TAG, "********** DownloadTask doInBackground");
//            Log.i(LOG_TAG, "********** DownloadTask count: " + count);
//            
//            if (myCloudDatasetId >=  0) {
//                long time1 = System.currentTimeMillis();
//                String fileName = "test" + count + ".jpg";
//                int result = mCcdiClient.doFileDownload(fileName, myCloudDatasetId);
//                long time2 = System.currentTimeMillis();
//                
//                String logString = "******** doFileDownload for /sdcard/acerDx/download/" + fileName + " result: " + result;
//    
//                builder.append(formatDisplayText(logString));
//                builder.append(NL);
//                builder.append("---- doFileDownload took : " + (time2 - time1) + " ms");
//                builder.append(NL);
//
//                Log.i(LOG_TAG, logString);
//            }
//          
//            return null;
//        }
//        
//        protected void onPostExecute(Void unused) {
//            mTextView.setText(builder);
    
    //    if(count < NUM_TIMES_TO_UPLOAD_DOWNLOAD)
    //      new DownloadTask(myCloudDatasetId, ++count).execute();
    //      else
    //      new DoUnlinkTask().execute();
//
//        }
//    }
    
    private class MediaClientTask extends AsyncTask<Void, Void, Void> {
      
        private boolean shouldCallHandleNextState = false;
        
        @Override
        protected Void doInBackground(Void... params) {
            Log.i(LOG_TAG, "********** MediaClientTask doInBackground");
            
            try
            {
                long initTime1 = System.currentTimeMillis();
                Log.i(LOG_TAG, "******** about to call mMcaClient.initForControlPoint " );
                File file = new File("/sdcard/acerDx/media");
                file.mkdirs();
                int result = mMcaClient.initForControlPoint("/sdcard/acerDx/media");
                long initTime2 = System.currentTimeMillis();
                
                String logString = "******** Mca initForControlPoint " + " result: " + result + " and it took: " + (initTime2 - initTime1) + " ms";
                Log.i(LOG_TAG, logString);
                builder.append(formatDisplayText(logString));
                builder.append(NL);
            }
            catch(Exception e)
            {
                Utils.logException(e);
                builder.append(formatDisplayText("@@****** Could not initialize McaClient@@"));
                builder.append(NL);
                return null;
            }
            
            long time1 = System.currentTimeMillis();
            
            JSONArray serverList = new JSONArray();
            
            long enumerateTime1 = System.currentTimeMillis();
            int result = mMcaClient.enumerateMediaServers(serverList);
            long enumerateTime2 = System.currentTimeMillis();
            
            String logString = "******** enumerateMediaServers " + " result: " + result + " and it took: " + (enumerateTime2 - enumerateTime1) + " ms";
            if(result != 0)
                logString = "******** enumerateMediaServers " + " result: @@" + result + "@@ and it took: " + (enumerateTime2 - enumerateTime1) + " ms";

            Log.i(LOG_TAG, logString);
            builder.append(formatDisplayText(logString));
            builder.append(NL);
            
            logString = "******** enumerateMediaServers serverList: " + serverList;
            Log.i(LOG_TAG, logString);
            builder.append(formatDisplayText(logString));
            builder.append(NL);
            
            JSONArray metadataList;
            JSONObject server, metadata;
            if( (serverList != null) && (serverList.length() > 0) )
            {
                for(int i = 0; i < serverList.length(); i++)
                {
                    try
                    {
                        server = serverList.getJSONObject(i);
                        metadataList = new JSONArray();
                        long getMetaDataTime1 = System.currentTimeMillis();
                        result = mMcaClient.getMetaData(server.getLong("CloudDeviceId"), 0, 0, //GetMetadataInput.MediaFlags.ALL_FILES, GetMetadataInput.SortFlags.NONE,  
                                metadataList);
                        long getMetaDataTime2 = System.currentTimeMillis();
                        
                        logString = "******** getMetaData " + " result: " + result + " and it took: " + (getMetaDataTime2 - getMetaDataTime1) + " ms";
                        if(result != 0)
                            logString = "******** getMetaData " + " result: @@" + result + "@@ and it took: " + (getMetaDataTime2 - getMetaDataTime1) + " ms";

                        Log.i(LOG_TAG, logString);
                        builder.append(formatDisplayText(logString));
                        builder.append(NL);
                        
                        logString = "******** getMetaData metadataList size: " + metadataList.length();
                        Log.i(LOG_TAG, logString);
                        builder.append(formatDisplayText(logString));
                        builder.append(NL);
                        
                        for(int j = 0; j < metadataList.length(); j++)
                        {
                            metadata = metadataList.getJSONObject(j);
                            if(metadata.has(MediaStore.Video.Media.TITLE) && "TestPhoto".equalsIgnoreCase(metadata.getString(MediaStore.Video.Media.TITLE)))
                            {
                                getMetaDataTime1 = System.currentTimeMillis();
                                String url = mMcaClient.getContentUrl(server.getLong("CloudDeviceId"), metadata.getString(MediaStore.MediaColumns._ID), 1);//GetContentUrlInput.UrlFlags.CONTENT_URL
                                getMetaDataTime2 = System.currentTimeMillis();
                                
                                logString = "******** getContentUrl of TestPhoto " + " url: " + url + " and it took: " + (getMetaDataTime2 - getMetaDataTime1) + " ms";
                                Log.i(LOG_TAG, logString);
                                builder.append(formatDisplayText(logString));
                                builder.append(NL);
                                
                                for( int k = 0; k < mNumberOfTimesToStreamMedia; k++)
                                {
                                    getMetaDataTime1 = System.currentTimeMillis();
                                    boolean httpGetResult = doHttpGet(url);
                                    getMetaDataTime2 = System.currentTimeMillis();
                                    
                                    logString = "******** doHttpGet of " + url + " result: " + httpGetResult + " and it took: " + (getMetaDataTime2 - getMetaDataTime1) + " ms";
                                    if( ! httpGetResult)
                                        logString = "******** doHttpGet of " + url + " result: @@" + httpGetResult + "@@ and it took: " + (getMetaDataTime2 - getMetaDataTime1) + " ms";
    
                                    Log.i(LOG_TAG, logString);
                                    builder.append(formatDisplayText(logString));
                                    builder.append(NL);
                                }
                            }
                        }
                        
                    }
                    catch(org.json.JSONException e)
                    {
                        Utils.logException(e);
                    }
                }
            }
            
            
            long time2 = System.currentTimeMillis();
            builder.append("---- Media Client test took : " + (time2 - time1) + " ms");
            builder.append(NL);

            Log.i(LOG_TAG, logString);
            
            return null;
        }
        
        protected void onPostExecute(Void unused) {
            
            mTextView.setText(builder);
            new DoUnlinkTask().execute();
        }
    }
    
    private boolean doHttpGet(String urlStr)
    {
        try
        {
            URL url = new URL(urlStr);
            URLConnection conn = url.openConnection();
            conn.setConnectTimeout(60000); //a full minute
            conn.setReadTimeout(60000);
    
            // Get the response
            BufferedReader rd = new BufferedReader(new InputStreamReader(conn.getInputStream()));
            StringBuffer sb = new StringBuffer();
            String line;
            while ((line = rd.readLine()) != null)
            {
                sb.append(line);
            }
            rd.close();
            Log.i(LOG_TAG, "******* reading input stream as string: " + sb.toString());

//            result = sb.toString();
            return true;
        } catch (Exception e)
        {
            Utils.logException(e);
        }
        
        return false;
    }
    
    private class DoUnlinkTask extends AsyncTask<Void, Void, Void> {

        @Override
        protected Void doInBackground(Void... params) {
            Log.i(LOG_TAG, "********** DoUnlinkTask doInBackground");
            
            long time1 = System.currentTimeMillis();
            int result = mCcdiClient.unlinkDevice();
            long time2 = System.currentTimeMillis();
            
            String logString = "******** unlinkDevice result: " + result;
            if(result != 0)
                logString = "******** unlinkDevice result: @@" + result + "@@";

//            builder.append(NL);
            builder.append(formatDisplayText(logString));
            builder.append(NL);
            builder.append("---- unlinkDevice took : " + (time2 - time1) + " ms");
            builder.append(NL);

            Log.i(LOG_TAG, logString);

            return null;
        }
        
        protected void onPostExecute(Void unused) {
            mTextView.setText(builder);
            
            new DoLogoutTask().execute();
        }
    }
    
    private class DoLogoutTask extends AsyncTask<Void, Void, Void> {

        @Override
        protected Void doInBackground(Void... params) {
            Log.i(LOG_TAG, "********** DoLogoutTask doInBackground");
            
            long time1 = System.currentTimeMillis();
            int result = mCcdiClient.doLogout();
            long time2 = System.currentTimeMillis();
            
            String logString = "******** doLogout result: " + result;
            if(result != 0)
                logString = "******** doLogout result: @@" + result + "@@";

//            builder.append(NL);
            builder.append(formatDisplayText(logString));
            builder.append(NL);
            builder.append("---- doLogout took : " + (time2 - time1) + " ms");
            builder.append(NL);

            Log.i(LOG_TAG, logString);

//            Log.i(LOG_TAG, "******** buffer : " + builder.toString());

            return null;
        }

        protected void onPostExecute(Void unused) {
            mTextView.setText(builder);

            handleNextState();
        }
    }
    
    private class DoLogUploadToOpsTask extends AsyncTask<Void, Void, Void> {
        
        private String logString;
        private String description;
        int result;
        
        public DoLogUploadToOpsTask(String logStr, String desc)
        {
            logString = logStr;
            description = desc;
        }

        @Override
        protected Void doInBackground(Void... params) {
            Log.i(LOG_TAG, "********** DoLogUploadToOpsTask doInBackground");
            
            long time1 = System.currentTimeMillis();
            result = mCcdiClient.uploadLog(description, logString);
            long time2 = System.currentTimeMillis();
            
            String logString = "******** DoLogUploadToOpsTask result: " + result + "---- DoLogUploadToOpsTask took : " + (time2 - time1) + " ms";
            if(result != 0)
                logString = "******** DoLogUploadToOpsTask result: @@" + result + "@@---- DoLogUploadToOpsTask took : " + (time2 - time1) + " ms";
            
            Log.i(LOG_TAG, logString);

            return null;
        }

        protected void onPostExecute(Void unused) {
            Toast.makeText(DXActivity.this, "Upload result: " + result, Toast.LENGTH_LONG).show();
        }
    }
    
    private void handleNextState()
    {
        if( ! mContinuousTestRunning ) //run once
        {
            setInitialState();
        }
        else if ( mContinuousTestRunning && ( ! mShouldStop) ) 
        {
            startAppropriateTask();
            //no other state change
        }
        else if ( mContinuousTestRunning && mShouldStop ) 
        {
            setInitialState();
        }
    }
    
    private void setInitialState()
    {
        mTypeOfTestButton.setEnabled(true);
        mLoggedInLoggedOutButton.setEnabled(true);
        mStartButton.setEnabled(true);
        mRunOnceButton.setEnabled(true);
        mStopButton.setEnabled(false);
        
        mContinuousTestRunning = false;
        mShouldStop = false;
        mFirstNLSkipped = false;
        
        mProgressBar.setVisibility(View.GONE);
    }
    
    private void showDescriptionDialog()
    {
        AlertDialog.Builder alert = new AlertDialog.Builder(this);

        alert.setMessage(R.string.enterDescription);

        // Set an EditText view to get user input 
        final EditText input = new EditText(this);
        alert.setView(input);

        alert.setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
        public void onClick(DialogInterface dialog, int whichButton) {
          new DoLogUploadToOpsTask(mTextView.getText().toString(), input.getText().toString()).execute();
          }
        });

        alert.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
          public void onClick(DialogInterface dialog, int whichButton) {
              Toast.makeText(DXActivity.this, R.string.uploadCancelled, Toast.LENGTH_LONG).show();
          }
        });

        alert.show();
    }
    
    private void showNumberOfTimesToStreamMediaDialog()
    {
        AlertDialog.Builder alert = new AlertDialog.Builder(this);

        alert.setMessage(R.string.NumberOfTimesToStreamMediaTitle);

        // Set an EditText view to get user input 
        final EditText input = new EditText(this);
        input.setText(String.valueOf(mNumberOfTimesToStreamMedia));
        alert.setView(input);

        alert.setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
        public void onClick(DialogInterface dialog, int whichButton) {
            try
            {
                int temp = Integer.parseInt(input.getText().toString());
                
                if( (temp < 1) || (temp > 1000) )
                {
                    handleShowNumberOfTimesToStreamMediaDialogError();
                    return;
                }
                
                mNumberOfTimesToStreamMedia = temp;
                
                SharedPreferences settings = getSharedPreferences(PREFS_NAME, MODE_PRIVATE);
                SharedPreferences.Editor editor = settings.edit();
                editor.putInt("numberOfTimesToStreamMedia", mNumberOfTimesToStreamMedia);
                editor.commit();
                
                Toast.makeText(DXActivity.this, "Each media item will be streamed: " + mNumberOfTimesToStreamMedia + " time(s) for the Media Client Test", Toast.LENGTH_LONG).show();
            }
            catch(NumberFormatException ex)
            {
                Utils.logException(ex);
                handleShowNumberOfTimesToStreamMediaDialogError();
            }
          }
        });

        alert.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
          public void onClick(DialogInterface dialog, int whichButton) {
              Toast.makeText(DXActivity.this, R.string.NumberOfTimesToStreamMediaCancelled, Toast.LENGTH_LONG).show();
          }
        });

        alert.show();
    }
    
    private void handleShowNumberOfTimesToStreamMediaDialogError()
    {
        Toast.makeText(DXActivity.this, R.string.NumberOfTimesToStreamMediaWrong, Toast.LENGTH_LONG).show();
        showNumberOfTimesToStreamMediaDialog();
    }
    
    private void showCustomGuidDialog()
    {
        AlertDialog.Builder alert = new AlertDialog.Builder(this);

        alert.setMessage(R.string.enterGuid);

        // Set an EditText view to get user input 
        final EditText input = new EditText(this);
        alert.setView(input);

        alert.setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
        public void onClick(DialogInterface dialog, int whichButton) {
            mCustomGuid = input.getText().toString();
            if(Utils.isStringNotEmpty(mCustomGuid))
                Toast.makeText(DXActivity.this, "Custom guid updated to: " + mCustomGuid + " \n Press 'Run Once' or 'Start' to download", Toast.LENGTH_LONG).show();
            else
                mCustomGuid = null;
          }
        });

        alert.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
          public void onClick(DialogInterface dialog, int whichButton) {
              Toast.makeText(DXActivity.this, getString(R.string.downloadGuidCancelled), Toast.LENGTH_LONG).show();
          }
        });

        alert.show();
    }

    private void showUsernamePasswordDialog()
    {
        AlertDialog.Builder alert = new AlertDialog.Builder(this);

        alert.setMessage(R.string.changeUserNamePassword);
        
        final View view = LayoutInflater.from(getBaseContext()).inflate(R.layout.credentials, null);
        EditText editText = (EditText)view.findViewById(R.id.username);
        editText.setText(mUsername);
        editText = (EditText)view.findViewById(R.id.password);
        editText.setText(mPassword);
        
        alert.setView(view);

        alert.setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
        public void onClick(DialogInterface dialog, int whichButton) {
            String username = ( (EditText) view.findViewById(R.id.username)).getText().toString();
            String password = ( (EditText) view.findViewById(R.id.password)).getText().toString();
            if(Utils.isStringNotEmpty(username))
                mUsername = username;
            if(Utils.isStringNotEmpty(password))
                mPassword = password;
            
            SharedPreferences settings = getSharedPreferences(PREFS_NAME, MODE_PRIVATE);
            SharedPreferences.Editor editor = settings.edit();

            editor.putString("username", mUsername);
            editor.putString("password", mPassword);
            editor.commit();
            
            Toast.makeText(DXActivity.this, "Username/Password updated to : " + mUsername + "/" + mPassword, Toast.LENGTH_LONG).show();
          }
        });

        alert.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
          public void onClick(DialogInterface dialog, int whichButton) {
              Toast.makeText(DXActivity.this, "Username/Password update cancelled", Toast.LENGTH_LONG).show();
          }
        });

        alert.show();
    }
    
    private static CharSequence formatDisplayText(String text)
    {
        CharSequence charSequence = setSpanBetweenTokens(text, "@@", new RelativeSizeSpan(1.5f), new ForegroundColorSpan(0xFFFFFFFF));
        charSequence = setSpanBetweenTokens(charSequence, "##", new RelativeSizeSpan(1.5f));
        
        return charSequence;
    }
    
    
    private static CharSequence setSpanBetweenTokens(CharSequence text, String token, CharacterStyle... cs)
    {
        // Start and end refer to the points where the span will apply
        int tokenLength = token.length();
        int startIndex = text.toString().indexOf(token);
        int endIndex = text.toString().indexOf(token, startIndex + 1);
        SpannableStringBuilder ssb = new SpannableStringBuilder(text);
        
//        Log.i(LOG_TAG, "******** setSpanBetweenTokens text: " + text);

        while ( (startIndex != -1) && (endIndex != -1) )
        {
//            Log.i(LOG_TAG, "******** setSpanBetweenTokens startIndex: " + startIndex + " endIndex: " + endIndex);

            for (CharacterStyle c : cs)
                ssb.setSpan(c, startIndex + tokenLength, endIndex, 0);

            // Delete the tokens before and after the span
            ssb.delete(endIndex, endIndex + tokenLength);
            ssb.delete(startIndex, startIndex + tokenLength);

            startIndex = ssb.toString().indexOf(token);
            endIndex = ssb.toString().indexOf(token, startIndex + 1);
        }

        return ssb;
    }
    
}