package com.igware.actool;


import java.io.BufferedReader;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.Scanner;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Environment;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.widget.EditText;
import android.widget.Toast;

public class ActoolActivity extends Activity {
    private static final String LOG_TAG = "ACTool";
    private EditText mDomainNameField;
    private EditText mUserGroupField;
    private EditText mBrandNameField;

    private String templateContent;
    private String confDirectoryPath;
    private String confFilePath;
    
    private static final String TEMPLATE_FILE = "ccd.conf.tmpl";
    private static final String CFG_FILE = "ccd.conf";

    // protocol of broadcasting
    private static final String ACTION = "action";
    private static final String ACTION_VALUE_DELETE = "delete";
    private static final String ACTION_VALUE_WRITE = "write";
    private static final String ACTION_VALUE_READ = "read";
    private static final String CONFIG = "config";
    private static final String RESULT_ACTION = "resultAction";
    private static final String RESULT = "result";
    private static final String GROUPRESULT = "groupresult";
    private static final String BRANDRESULT = "brandresult";
    
    private static final String SDCARD_DIRECTORY_PATH = Environment.getExternalStorageDirectory().getAbsolutePath();
    private static final String DEFAULT_CONF_PATH = SDCARD_DIRECTORY_PATH + "/AOP/AcerCloud/conf/" + CFG_FILE;

    ToolReceiver receiver = new ToolReceiver();

    String sendConfigAction = "com.igware.actool.CONFIG_BROADCAST";
    String receiveResultAction = "com.igware.acpanel.RESULT_BROADCAST";
    

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.actool);

        mDomainNameField = (EditText) findViewById(R.id.EditText_DomainName);
        mUserGroupField = (EditText) findViewById(R.id.EditText_UserGroup);
        mBrandNameField = (EditText) findViewById(R.id.EditText_BrandName);
    }

    @Override
    protected void onStart() {
        super.onStart();
        getDomain();
    }

    @Override
    protected void onStop() {
        super.onStop();
    }
    
    private class ToolReceiver {

        public void handleOnReceive(Context context, Intent intent) {

            Log.d(LOG_TAG, "in onReceive, intent action: " + intent.getAction()
                    + " intent: " + intent);

            // extract "result"
            Bundle extras = intent.getExtras();
            String action = extras != null ? extras.getString(ACTION) : null;
            String result = extras != null ? extras.getString(RESULT) : null;
            String groupresult = extras != null ? extras.getString(GROUPRESULT) : null;
            String brandresult = extras != null ? extras.getString(BRANDRESULT) : null;

            if (action!=null && action.equals(ACTION_VALUE_READ)) {
                //update ActoolActivity to display updated domain name
                mDomainNameField.setText(result);
                mUserGroupField.setText(groupresult);
                mBrandNameField.setText(brandresult);
            } else {
                Toast.makeText(context, result, Toast.LENGTH_SHORT).show();
            }
        }
    }

    public void onClick_Save(View view) {
        // use template file to generate config file and overwrite
        String domainInput = mDomainNameField.getText().toString();
        String brandNameInput = mBrandNameField.getText().toString();
        if (TextUtils.isEmpty(domainInput)) {
            domainInput = "cloud.acer.com";
            mDomainNameField.setText(domainInput);
        }
        if (TextUtils.isEmpty(brandNameInput)) {
        	brandNameInput = "AcerCloud";
            mBrandNameField.setText(brandNameInput);
        }
        confDirectoryPath = SDCARD_DIRECTORY_PATH + "/AOP/" +brandNameInput + "/conf/";
        confFilePath = SDCARD_DIRECTORY_PATH + "/AOP/" +brandNameInput + "/conf/"+ CFG_FILE;
        String groupInput = mUserGroupField.getText().toString();
        updateDomainNameAndUserGroup(domainInput, groupInput);
    }

    public void onClick_Delete(View view) {
        String brandNameInput = mBrandNameField.getText().toString();
        if (TextUtils.isEmpty(brandNameInput)) {
        	brandNameInput = "AcerCloud";
            mBrandNameField.setText(brandNameInput);
        }
        confFilePath = SDCARD_DIRECTORY_PATH + "/AOP/" +brandNameInput + "/conf/" + CFG_FILE;
        Intent intent = new Intent(sendConfigAction);
        intent.putExtra(ACTION, ACTION_VALUE_DELETE);
        intent.putExtra(RESULT_ACTION, receiveResultAction);
        handleIntent(this, intent);
    }

    public void onClick_Cancel(View view) {
        String brandNameInput = mBrandNameField.getText().toString();
        if (TextUtils.isEmpty(brandNameInput)) {
        	brandNameInput = "AcerCloud";
            mBrandNameField.setText(brandNameInput);
        }
        confFilePath = SDCARD_DIRECTORY_PATH + "/AOP/" +brandNameInput + "/conf/" + CFG_FILE;
        getDomain();
    }

    private void getDomain() {
        Intent intent = new Intent(sendConfigAction);
        intent.putExtra(ACTION, ACTION_VALUE_READ);
        intent.putExtra(RESULT_ACTION, receiveResultAction);
        handleIntent(this, intent);
    }

    private void updateDomainNameAndUserGroup(String newValue, String userGroup) {
        if (templateContent == null) {
            Scanner scanner = null;
            StringBuilder sb = new StringBuilder();
            try {
                final InputStream is = getResources().getAssets().open(
                        TEMPLATE_FILE);
                String NL = System.getProperty("line.separator");
                scanner = new Scanner(is, "UTF-8");
                while (scanner.hasNextLine()) {
                    sb.append(scanner.nextLine() + NL);
                }

                templateContent = sb.toString();

            } catch (IOException ioe) {
                StringWriter sw = new StringWriter();
                PrintWriter pw = new PrintWriter(sw);
                ioe.printStackTrace(pw);
                Log.e(LOG_TAG, sw.toString());
            }
        }

        String newContent = templateContent.replace("${DOMAIN}", newValue);
        newContent = newContent.replace("${GROUP}", userGroup);
        Log.d(LOG_TAG, "New config content: " + newContent);
        Intent intent = new Intent(sendConfigAction);
        intent.putExtra(ACTION, ACTION_VALUE_WRITE);
        intent.putExtra(CONFIG, newContent);
        intent.putExtra(RESULT_ACTION, receiveResultAction);
        handleIntent(this, intent);
    }
    
    private void handleIntent(Context context, Intent intent) {

        Log.d(LOG_TAG, "in onReceive, intent action: " + intent.getAction()
                + " intent: " + intent);
        boolean success = false;

        // extract config content
        Bundle extras = intent.getExtras();
        String config = extras != null ? extras.getString(CONFIG) : null;
        String action = extras != null ? extras.getString(ACTION) : null;
        String resultaction = extras != null ? extras.getString(RESULT_ACTION)
                : null;

        String resultMsg = null;
        String userGroup = null;
        String brandName = null;
        if (action.equals(ACTION_VALUE_DELETE)) {
            Log.d(LOG_TAG, "action == delete");
            // delete config file
            File cfgFile = new File(confFilePath);
            if (cfgFile.exists()) {
                if (cfgFile.delete()) {
                    resultMsg = "config file deleted.";
                    Log.d(LOG_TAG, "config file deleted.");
                } else {
                    resultMsg = "Failed to delete config file.";
                    Log.d(LOG_TAG, "Failed to delete config file.");
                }
            } else {
                resultMsg = "config file doesn't exist. Nothing to delete.";
                Log.d(LOG_TAG, "config file doesn't exist. Nothing to delete.");
            }
        }

        else if (action.equals(ACTION_VALUE_WRITE)) {
            Log.d(LOG_TAG, "action == write");
            try {
                // write to internal file
                File dirFile = new File(confDirectoryPath);
                if ((!dirFile.exists()) && dirFile.mkdirs()) {
                    Log.d(LOG_TAG, "Dir created");
                }
                else if (!dirFile.exists())
                {
                    resultMsg = "cannot create directory: " + confDirectoryPath;
                    Log.e(LOG_TAG, resultMsg);
                }
                
                File cfgFile = new File(confFilePath);
                if (cfgFile.createNewFile()) {
                    Log.d(LOG_TAG, "File created.");
                }

                OutputStream out = new FileOutputStream(cfgFile);
                out.write(config.getBytes());
                out.close();

                // verify write success by reading it back
                InputStream in = new FileInputStream(cfgFile);
                DataInputStream din = new DataInputStream(in);
                BufferedReader br = new BufferedReader(new InputStreamReader(
                        din));
                String strLine;
                while ((strLine = br.readLine()) != null) {
                    Log.d(LOG_TAG, strLine);
                }
                in.close();
                din.close();
                success = true;
                Log.d(LOG_TAG, "Successfully written config file.");
                resultMsg = success ? "Written to config file"
                        : "Failed to write config file";
            } catch (Exception e) {
                StringWriter sw = new StringWriter();
                PrintWriter pw = new PrintWriter(sw);
                e.printStackTrace(pw);
                Log.e(LOG_TAG, sw.toString());
            }
        } else if (action.equals(ACTION_VALUE_READ)) {
            Log.d(LOG_TAG, "action == read");

            File cfgFile = new File(DEFAULT_CONF_PATH);
            if (cfgFile.exists()) {
                try {
                    InputStream in = new FileInputStream(cfgFile);
                    DataInputStream din = new DataInputStream(in);
                    BufferedReader br = new BufferedReader(
                            new InputStreamReader(din));
                    String strLine;
                    while ((strLine = br.readLine()) != null) {
                        if (strLine.contains("infraDomain")) {
                            int start = strLine.indexOf('=')+1;
                            resultMsg = strLine.substring(start);
                            if(resultMsg != null)
                                resultMsg = resultMsg.trim();
                        }
                        if (strLine.contains("userGroup")) {
                            int start = strLine.indexOf('=')+1;
                            userGroup = strLine.substring(start);
                            if(userGroup != null)
                            	userGroup = userGroup.trim();
                        }
                    }
                    in.close();
                    din.close();
                } catch (Exception e) {
                    StringWriter sw = new StringWriter();
                    PrintWriter pw = new PrintWriter(sw);
                    e.printStackTrace(pw);
                    Log.e(LOG_TAG, sw.toString());
                }
            }
        } else {
            Log.e(LOG_TAG, "Unrecognized action");
        }

        // send back result
        Intent result = new Intent(resultaction);
        result.putExtra(RESULT, resultMsg);
        result.putExtra(GROUPRESULT, userGroup);
        result.putExtra(BRANDRESULT, brandName);
        result.putExtra(ACTION, action);
        receiver.handleOnReceive(context, result);
    }
    
}
