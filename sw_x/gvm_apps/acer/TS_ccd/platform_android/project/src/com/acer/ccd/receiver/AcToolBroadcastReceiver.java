package com.acer.ccd.receiver;

import java.io.BufferedReader;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.io.UnsupportedEncodingException;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;

public class AcToolBroadcastReceiver extends BroadcastReceiver {

	private static final String TAG = "AcToolBroadcastReceiver";
	private static final String CFG_FILE = "ccd.conf";

	// keys for communication with actool
	private static final String CONFIG = "config";
	private static final String ACTION = "action";
	private static final String ACTION_VALUE_DELETE = "delete";
	private static final String ACTION_VALUE_WRITE = "write";
	private static final String ACTION_VALUE_READ = "read";
	private static final String RESULT_ACTION = "resultAction";
	private static final String RESULT = "result";

	@Override
	public void onReceive(Context context, Intent intent) {

		Log.d(TAG, "in onReceive, intent action: " + intent.getAction()
				+ " intent: " + intent);
		boolean success = false;

		// extract config content
		Bundle extras = intent.getExtras();
		String config = extras != null ? extras.getString(CONFIG) : null;
		String action = extras != null ? extras.getString(ACTION) : null;
		String resultaction = extras != null ? extras.getString(RESULT_ACTION)
				: null;

		String resultMsg = null;
		if (action.equals(ACTION_VALUE_DELETE)) {
			Log.d(TAG, "action == delete");
			// delete config file
			File cfgFile = new File(context.getFilesDir().getPath() + "/conf/"
					+ CFG_FILE);
			if (cfgFile.exists()) {
				if (cfgFile.delete()) {
					resultMsg = "config file deleted.";
					Log.d(TAG, "config file deleted.");
				} else {
					resultMsg = "Failed to delete config file.";
					Log.d(TAG, "Failed to delete config file.");
				}
			} else {
				resultMsg = "config file doesn't exist. Nothing to delete.";
				Log.d(TAG, "config file doesn't exist. Nothing to delete.");
			}
		}

		else if (action.equals(ACTION_VALUE_WRITE)) {
			Log.d(TAG, "action == write");
			try {
				// write to internal file
				File dirFile = new File(context.getFilesDir().getPath()
						+ "/conf");
				if (dirFile.mkdir()) {
					Log.d(TAG, "Dir created");
				}
				File cfgFile = new File(context.getFilesDir().getPath()
						+ "/conf/" + CFG_FILE);
				if (cfgFile.createNewFile()) {
					Log.d(TAG, "File created.");
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
					Log.d(TAG, strLine);
				}
				in.close();
				din.close();
				success = true;
				Log.d(TAG, "Successfully written config file.");
				resultMsg = success ? "Written to config file"
						: "Failed to write config file";
			} catch (Exception e) {
				StringWriter sw = new StringWriter();
				PrintWriter pw = new PrintWriter(sw);
				e.printStackTrace(pw);
				Log.e(TAG, sw.toString());
			}
		} else if (action.equals(ACTION_VALUE_READ)) {
			Log.d(TAG, "action == read");

			File cfgFile = new File(context.getFilesDir().getPath() + "/conf/"
					+ CFG_FILE);
			if (cfgFile.exists()) {
				try {
					InputStream in = new FileInputStream(cfgFile);
					DataInputStream din = new DataInputStream(in);
					BufferedReader br = new BufferedReader(
							new InputStreamReader(din));
					String strLine;
					while ((strLine = br.readLine()) != null) {
						if (strLine.contains("vsdsHostName")) {
							int start = strLine.indexOf('.')+1;
							resultMsg = strLine.substring(start);
						}
					}
					in.close();
					din.close();
				} catch (Exception e) {
					StringWriter sw = new StringWriter();
					PrintWriter pw = new PrintWriter(sw);
					e.printStackTrace(pw);
					Log.e(TAG, sw.toString());
				}
			}
		} else {
			Log.e(TAG, "Unrecognized action");
		}

		// send back result
		Intent result = new Intent(resultaction);
		result.putExtra(RESULT, resultMsg);
		result.putExtra(ACTION, action);
		context.sendBroadcast(result);
	}
}