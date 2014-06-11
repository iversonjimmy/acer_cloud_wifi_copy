//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
package com.igware.example_panel.ui;

import java.io.File;
import java.util.Arrays;
import java.util.Hashtable;
import java.util.List;

import android.app.ProgressDialog;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Log;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.AdapterView.AdapterContextMenuInfo;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import com.igware.android_cc_sdk.example_panel_and_service.R;
import com.igware.example_panel.util.Constants;
import com.igware.example_panel.util.Dataset;
import com.igware.example_panel.util.Filter;
import com.igware.example_panel.util.Subfolder;
import com.igware.example_panel.util.Subfolder.Type;
import com.igware.example_panel.util.SyncListItem;

/**
 * Display and allow editing sync folders
 * 
 * @author cindy
 */
public class SyncFoldersActivity extends CcdListActivity {
    private static final String LOG_TAG = "SyncFoldersActivity";
    private static final String DATASET_ID = "_id";
    private static final String PATH = "_path";
    private static final String DATASET_NAME = "_name";

    private static Hashtable<String, String> MIMELookup = new Hashtable<String, String>();

    // for retrieving folder structure
    private long datasetId;
    private String mPath;
    private String datasetName;
    private boolean restarted;
    private MyArrayAdapter adapter;

    private ProgressDialog mProgressDialog;
    private Button parentButton;
    private Button saveButton;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mProgressDialog = ProgressDialog.show(this, "",
                getText(R.string.Msg_Working), true);

        setContentView(R.layout.syncfolders);
        registerForContextMenu(getListView());
        setTitle(R.string.Label_SyncFolders);

        Bundle extras = getIntent().getExtras();
        datasetId = extras != null ? extras.getLong(DATASET_ID) : -1;
        mPath = extras != null ? extras.getString(PATH) : null;
        datasetName = extras != null ? extras.getString(DATASET_NAME) : null;

        this.parentButton = (Button) this.findViewById(R.id.parent);
        if (datasetName != null) {
            parentButton.setText(datasetName);
        }
        if (datasetId > 0) {
            parentButton.setEnabled(true);
            String tail = Filter.getTailFolder(mPath);
            if (tail != null) {
                parentButton.setText(tail);
            }
        } else {
            parentButton.setEnabled(false);
        }
        this.parentButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                // TODO: Reload parent activity with subscription saved in
                // memory.
                finish();
            }
        });

        this.saveButton = (Button) this.findViewById(R.id.save);
        this.saveButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                mProgressDialog = ProgressDialog.show(SyncFoldersActivity.this,
                        "", getText(R.string.Msg_Working), true);

                new SaveChangeTask().execute();
            }
        });

        initiateMIMETypeTable();
    }

    @Override
    protected void onStart() {

        super.onStart();
        // Only when we are browsing sub-folders shall we reload
        // When we're at top level, don't call backend again.
        if (!restarted || restarted && datasetId > 0) {
            new GetSyncFoldersTask().execute(0);
        } else {
            if (adapter!=null) {
            adapter.notifyDataSetChanged();
            }
        }
    }

    @Override
    protected void onStop() {
        super.onStop();
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (adapter != null) {
            adapter.notifyDataSetChanged();
        }
    }

    @Override
    protected void onRestart() {
        super.onRestart();
        restarted = true;
    }

    @Override
    protected void onPause() {
        super.onPause();
        mProgressDialog.dismiss();
    }

    private class GetSyncFoldersTask extends
            AsyncTask<Integer, Void, SyncListItem[]> {

        @Override
        protected SyncListItem[] doInBackground(Integer... params) {
            if (params != null && params.length > 0 && params[0] != null) {
                int sleepTimeMs = params[0].intValue();
                if (sleepTimeMs > 0) {
                    try {
                        Thread.sleep(sleepTimeMs);
                    } catch (InterruptedException e) {
                        Log.i(LOG_TAG, "Spurious wakeup");
                    }
                }
            }

            if (datasetId > 0) {
                SyncListItem[] subFolders = mBoundService.listSyncItems(
                        datasetId, mPath);
                Arrays.sort(subFolders);
                return subFolders;
            } else {
                if (!restarted) {
                    Dataset[] syncFoldersArray = mBoundService
                            .listOwnedDataSets();
                    Dataset[] subscribedFolders = mBoundService
                            .listSyncSubscriptions();
                    processSyncFolders(syncFoldersArray, subscribedFolders);
                    return syncFoldersArray;
                }
                return null;
            }
        }

        @Override
        protected void onPostExecute(SyncListItem[] result) {
            if (mProgressDialog.isShowing()) {
                mProgressDialog.dismiss();
            }

            if (result != null && result.length > 0) {
                List<SyncListItem> resultList = Arrays.asList(result);
                adapter = new MyArrayAdapter(SyncFoldersActivity.this,
                        R.layout.syncfolder_row, R.id.Text_SyncFolderRow,
                        resultList);

                setListAdapter(adapter);
            }
        }
    }

    private void initiateMIMETypeTable() {
        MIMELookup.put("bmp", "image/*");
        MIMELookup.put("cod", "image/*");
        MIMELookup.put("gif", "image/*");
        MIMELookup.put("ief", "image/*");
        MIMELookup.put("jpe", "image/*");
        MIMELookup.put("jpeg", "image/*");
        MIMELookup.put("jpg", "image/*");
        MIMELookup.put("jfif", "image/*");
        MIMELookup.put("png", "image/*");
        MIMELookup.put("svg", "image/*");
        MIMELookup.put("tif", "image/*");
        MIMELookup.put("tiff", "image/*");
        MIMELookup.put("ras", "image/*");
        MIMELookup.put("cmx", "image/*");
        MIMELookup.put("ico", "image/*");
        MIMELookup.put("pnm", "image/*");
        MIMELookup.put("pbm", "image/*");
        MIMELookup.put("pgm", "image/*");
        MIMELookup.put("rgb", "image/*");
        MIMELookup.put("xbm", "image/*");
        MIMELookup.put("xpm", "image/*");
        MIMELookup.put("xwd", "image/*");

        MIMELookup.put("mp2", "video/*");
        MIMELookup.put("mpa", "video/*");
        MIMELookup.put("mpe", "video/*");
        MIMELookup.put("mpeg", "video/*");
        MIMELookup.put("mpg", "video/*");
        MIMELookup.put("mpv2", "video/*");
        MIMELookup.put("mov", "video/*");
        MIMELookup.put("qt", "video/*");
        MIMELookup.put("lsf", "video/*");
        MIMELookup.put("lsx", "video/*");
        MIMELookup.put("asf", "video/*");
        MIMELookup.put("asr", "video/*");
        MIMELookup.put("asx", "video/*");
        MIMELookup.put("avi", "video/*");
        MIMELookup.put("movie", "video/*");

        MIMELookup.put("css", "text/*");
        MIMELookup.put("323", "text/*");
        MIMELookup.put("htm", "text/*");
        MIMELookup.put("html", "text/*");
        MIMELookup.put("stm", "text/*");
        MIMELookup.put("uls", "text/*");
        MIMELookup.put("bas", "text/*");
        MIMELookup.put("c", "text/*");
        MIMELookup.put("h", "text/*");
        MIMELookup.put("txt", "text/*");
        MIMELookup.put("rtx", "text/*");
        MIMELookup.put("sct", "text/*");
        MIMELookup.put("tsv", "text/*");
        MIMELookup.put("htt", "text/*");
        MIMELookup.put("htc", "text/*");
        MIMELookup.put("etx", "text/*");
        MIMELookup.put("vcf", "text/*");

        MIMELookup.put("au", "audio/*");
        MIMELookup.put("snd", "audio/*");
        MIMELookup.put("mid", "audio/*");
        MIMELookup.put("rmi", "audio/*");
        MIMELookup.put("mp3", "audio/*");
        MIMELookup.put("aif", "audio/*");
        MIMELookup.put("aifc", "audio/*");
        MIMELookup.put("aiff", "audio/*");
        MIMELookup.put("m3u", "audio/*");
        MIMELookup.put("ra", "audio/*");
        MIMELookup.put("ram", "audio/*");
        MIMELookup.put("wav", "audio/*");

        MIMELookup.put("pdf", "application/pdf");
    }

    private class MyArrayAdapter extends ArrayAdapter {
        public MyArrayAdapter(Context context, int resource,
                int textViewResourceId, List<SyncListItem> objects) {
            super(context, resource, textViewResourceId, objects);
            // TODO Auto-generated constructor stub
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            SyncListItem folder = (SyncListItem) this.getItem(position);
            View vi = convertView;

            if (convertView == null) {
                LayoutInflater inflater = (LayoutInflater) this.getContext()
                        .getSystemService(Context.LAYOUT_INFLATER_SERVICE);
                vi = inflater.inflate(R.layout.syncfolder_row, null);
            }

            // TODO: use ViewHolder to avoid findViewById calls
            ImageView image = (ImageView) vi
                    .findViewById(R.id.Image_SyncFolderImage);

            switch (folder.getSubscriptionState()) {
            case SUBSCRIBED:
                image.setImageResource(R.drawable.checked);
                break;
            case UNSUBSCIRBED:
                image.setImageResource(R.drawable.unchecked);
                break;
            case PARTIALLY_SUBSCRIBED:
                image.setImageResource(R.drawable.partial);
            }

            image.setClickable(true);
            image.setOnClickListener(new OnImageClickListener(folder));

            // TODO: use ViewHolder to avoid findViewById calls
            TextView text = (TextView) vi.findViewById(R.id.Text_SyncFolderRow);
            text.setText(folder.getName());
            text.setClickable(true);

            text.setOnClickListener(new OnTextClickListener(folder));

            return vi;
        }
    }

    private class OnImageClickListener implements OnClickListener {
        private SyncListItem mFolder;

        OnImageClickListener(SyncListItem folder) {
            mFolder = folder;
        }

        @Override
        public void onClick(View arg0) {
            switch (mFolder.getSubscriptionState()) {
            case SUBSCRIBED:
                if (mFolder instanceof Subfolder) {
                    mBoundService.specify(((Subfolder) mFolder).getDataset(),
                            (Subfolder) mFolder);
                }

                mFolder.unsubscribe();

                if (mFolder instanceof Subfolder) {

                    String path = ((Subfolder) mFolder).getPath();
                    Dataset ds = ((Subfolder) mFolder).getDataset();

                    do {
                        if (ds.allSubfoldersUnSubscribed(path)) {
                            ds.invalidatePath(path);
                            path = Filter.stripTailFolder(path);
                        } else {
                            break;
                        }
                    } while (Filter.getDepth(path) > 0);
                }

                ((ImageView) arg0).setImageResource(R.drawable.unchecked);
                break;
            case UNSUBSCIRBED:
                mFolder.subscribe();
                if (mFolder instanceof Subfolder) {

                    String path = ((Subfolder) mFolder).getPath();
                    Dataset ds = ((Subfolder) mFolder).getDataset();

                    do {
                        if (mBoundService.allSubfoldersCompletelySubscribed(ds,
                                path)) {
                            ds.nullifyFilter(path);
                            path = Filter.stripTailFolder(path);
                        } else {
                            break;
                        }
                    } while (Filter.getDepth(path) > 0);
                }
                ((ImageView) arg0).setImageResource(R.drawable.checked);
                break;
            case PARTIALLY_SUBSCRIBED:
                mFolder.partialToAll();
                if (mFolder instanceof Subfolder) {

                    String path = ((Subfolder) mFolder).getPath();
                    Dataset ds = ((Subfolder) mFolder).getDataset();

                    do {
                        if (mBoundService.allSubfoldersCompletelySubscribed(ds,
                                path)) {
                            ds.nullifyFilter(path);
                            path = Filter.stripTailFolder(path);
                        } else {
                            break;
                        }
                    } while (Filter.getDepth(path) > 0);
                }
                ((ImageView) arg0).setImageResource(R.drawable.checked);
                break;
            }
        }
    }

    private class OnTextClickListener implements OnClickListener {
        private SyncListItem mFolder;

        OnTextClickListener(SyncListItem folder) {
            mFolder = folder;
        }

        @Override
        public void onClick(View arg0) {
            Intent subfolderIntent = new Intent(SyncFoldersActivity.this,
                    SyncFoldersActivity.class);

            if (mFolder instanceof Subfolder) {
                if (((Subfolder) mFolder).getType() == Type.Folder) {
                    subfolderIntent.putExtra(PATH,
                            ((Subfolder) mFolder).getFolderFullPath());
                    subfolderIntent.putExtra(DATASET_ID, ((Subfolder) mFolder)
                            .getDataset().getDatasetId());
                    startActivityForResult(subfolderIntent,
                            Constants.ACTIVITY_SYNCFOLDERS);
                } else {
                    Subfolder mfile = (Subfolder) mFolder;
                    File file = new File(mfile.getAbsoluteFolderDevicePath());
                    Log.d(LOG_TAG,
                            "*****Rending "
                                    + mfile.getAbsoluteFolderDevicePath());
                    String mimeType = mapMIMEType(mfile.getName());

                    if (!file.exists()) {
                        Toast.makeText(SyncFoldersActivity.this,
                                "File doesn't exist", Toast.LENGTH_SHORT)
                                .show();
                    } else if (mimeType != null) {
                        Uri path = Uri.fromFile(file);
                        Intent intent = new Intent(Intent.ACTION_VIEW);
                        intent.setDataAndType(path, mimeType);
                        intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);

                        try {
                            startActivity(intent);
                        } catch (ActivityNotFoundException e) {
                            Toast.makeText(SyncFoldersActivity.this,
                                    "No Application Available to View File",
                                    Toast.LENGTH_SHORT).show();
                        }
                    }
                }

            } else if (mFolder instanceof Dataset) {
                subfolderIntent.putExtra(DATASET_ID,
                        ((Dataset) mFolder).getDatasetId());
                subfolderIntent.putExtra(DATASET_NAME,
                        ((Dataset) mFolder).getName());
                startActivityForResult(subfolderIntent,
                        Constants.ACTIVITY_SYNCFOLDERS);
            }
        }
    }

    private static String mapMIMEType(String filename) {
        int lastDotIndex = filename.lastIndexOf(".");
        if (lastDotIndex > 0) {
            String ext = filename.substring(lastDotIndex + 1).toLowerCase();
            return MIMELookup.get(ext);
        }
        return null;
    }

    protected SyncListItem getItemFromMenu(ContextMenuInfo menuInfo) {
        Log.d(LOG_TAG, "getItemFromMenu");
        AdapterContextMenuInfo info = (AdapterContextMenuInfo) menuInfo;
        SyncListItem selected = (SyncListItem) getListAdapter().getItem(
                info.position);
        return selected;
    }

    /*
    * for every item in listview, call backend to update subscription
    */
    private class SaveChangeTask extends AsyncTask<Void, Void, Boolean> {
        @Override
        protected Boolean doInBackground(Void... params) {

            return mBoundService.savechange();

        }

        @Override
        protected void onPostExecute(Boolean success) {

            if (mProgressDialog.isShowing()) {
                mProgressDialog.dismiss();
            }
            if (success) {
                Toast.makeText(SyncFoldersActivity.this, "Changes saved!",
                        Toast.LENGTH_SHORT).show();
            } else {
                Toast.makeText(SyncFoldersActivity.this,
                        "Failed to save changes.", Toast.LENGTH_SHORT).show();
            }
        }

    }

    /*
    * Using id as the key, if item from a1 exists in a2, mark it as subscribed.
    */
    private void processSyncFolders(Dataset[] a1, Dataset[] a2) {
        Hashtable<Long, Dataset> table = new Hashtable<Long, Dataset>();
        for (int i = 0; i < a2.length; i++) {
            table.put(a2[i].getDatasetId(), a2[i]);
        }
        for (int i = 0; i < a1.length; i++) {
            Dataset.StoreDataset(a1[i]);
            if (table.containsKey(a1[i].getDatasetId())) {
                a1[i].setHasSubscription();
                a1[i].setFilterString(table.get(a1[i].getDatasetId())
                        .getFilterString());
                a1[i].setAbsoluteDeviceRoot(table.get(a1[i].getDatasetId())
                        .getAbsoluteDeviceRoot());
            }
        }
        // now sort
        Arrays.sort(a1);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {

        Log.i(LOG_TAG, "Result from request " + requestCode + ": " + resultCode);

        switch (requestCode) {
        case Constants.ACTIVITY_SYNCFOLDERS:
            if (resultCode == RESULT_CANCELED) {
                // The child activity canceled out. Do nothing.
            } else {
                if (resultCode != RESULT_OK
                        && resultCode != Constants.RESULT_RESTART
                        && resultCode != Constants.RESULT_EXIT
                        && resultCode != Constants.RESULT_BACK_TO_MAIN) {
                    Log.e(LOG_TAG, "Encountered unknown result code: "
                            + resultCode);
                }
                // Pass the result back up to the parent activity.
                setResult(resultCode, data);
                finish();
            }
            break;

        default:
            super.onActivityResult(requestCode, resultCode, data);
        }
    }

    /**
    * @see android.app.Activity#onContextItemSelected(android.view.MenuItem)
    */
    /*
    * @Override public boolean onContextItemSelected(MenuItem item) { Folder
    * selected = getItemFromMenu(item.getMenuInfo());
    * Toast.makeText(SyncFoldersActivity.this, "onContextItemSelected!",
    * Toast.LENGTH_SHORT).show(); return super.onContextItemSelected(item); }
    */
}
