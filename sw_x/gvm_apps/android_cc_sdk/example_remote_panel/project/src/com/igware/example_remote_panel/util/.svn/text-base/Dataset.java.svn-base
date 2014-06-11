package com.igware.example_remote_panel.util;

import java.util.ArrayList;
import java.util.Enumeration;
import java.util.Hashtable;

import android.util.Log;

public class Dataset extends SyncListItem {
    private static final String LOG_TAG = "Dataset";
    private long datasetId;
    private boolean originallySubscribed;
    private boolean isSubscribed;
    private String filterString;
    private Filter[] filters;// dataset has a list of filters.
    private String absoluteDeviceRoot;

    public String getAbsoluteDeviceRoot() {
        return absoluteDeviceRoot;
    }

    public void setAbsoluteDeviceRoot(String absoluteDeviceRoot) {
        this.absoluteDeviceRoot = absoluteDeviceRoot;
    }

    // We need to use datasetId to lookup a dataset
    private static Hashtable<Long, Dataset> datasetLookupTable;

    static {
        datasetLookupTable = new Hashtable<Long, Dataset>();
    }

    public long getDatasetId() {
        return datasetId;
    }

    @Override
    public State getSubscriptionState() {
        if (isSubscribed) {
            if (filters == null) {
                return State.SUBSCRIBED;
            }
            return State.PARTIALLY_SUBSCRIBED;
        }
        return State.UNSUBSCIRBED;
    }

    /*
     * Definition of states: --For subfolder-- <p> if .dataset is UNSUBSCIRBED -
     * UNSUBSCIRBED <p> if .dataset is SUBSCRIBED - SUBSCRIBED <p> if .dataset
     * is PARTIALLY_SUBSCRIBED - call .dataset.isVisible <p> if isVisible if
     * filter is not null - PARTIALLY SUBSCRIBED <p> if filter is null -
     * SUBSCRIBED <p> if !isVisible - UNSUBSCIRBED
     */
    public State getSubScriptionStateForSubFolder(String path, String folderName) {

        if (getSubscriptionState() == State.SUBSCRIBED) {
            Log.d(LOG_TAG, "getSubScriptionStateForSubFolder: " + path + "/"
                    + folderName + " state: subscribed. dorminated by dataset");
            return State.SUBSCRIBED;
        }
        if (getSubscriptionState() == State.UNSUBSCIRBED) {
            Log.d(LOG_TAG, "getSubScriptionStateForSubFolder: " + path + "/"
                    + folderName
                    + " state: unsubscribed. dornimated by dataset");
            return State.UNSUBSCIRBED;
        }

        if (filters == null) {
            Log.e(LOG_TAG,
                    "Should already return State.Subscribed instead of coming here");
        }

        for (Filter filter : filters) {
            State t = filter.isVisible(path, folderName);
            if (t != State.UNSUBSCIRBED) {
                Log.d(LOG_TAG, "getSubScriptionStateForSubFolder: " + path
                        + "/" + folderName + " state: " + t);
                return t;
            }
        }
        Log.d(LOG_TAG, "getSubScriptionStateForSubFolder: " + path + "/"
                + folderName + " state: unsubscribed");
        return State.UNSUBSCIRBED;
    }

    @Override
    public void subscribe() {
        setIsSubscribed(true);
    }

    @Override
    public void unsubscribe() {
        setIsSubscribed(false);
    }

    @Override
    public void partialToAll() {
        filters = null;
        filterString = null;
        setIsSubscribed(true);
    }

    public void setHasSubscription() {
        originallySubscribed = true;
        isSubscribed = true;
    }

    public boolean isOriginallySubscribed() {
        return originallySubscribed;
    }

    private void setIsSubscribed(boolean sub) {
        // When subscription is false, filters are automatically invalidated.
        if (!sub) {
            filters = null;
            filterString = null;
        }

        assert filters == null;
        assert filterString == null;

        isSubscribed = sub;
    }

    public Dataset(long i, String n) {
        datasetId = i;
        name = n;
        isSubscribed = false;
    }

    public static void StoreDataset(Dataset ds) {
        Dataset.datasetLookupTable.put(ds.datasetId, ds);
    }

    public void setFilterString(String s) {
        filterString = s;
        filters = Filter.ParseFilterString(filterString);
    }

    public String getFilterString() {
        return filterString;
    }

    public Filter[] getFilters() {
        if (filters == null && filterString != null
                && filterString.length() > 0) {
            filters = Filter.ParseFilterString(filterString);
        }
        return filters;
    }

    /*
     * A dataset keeps a string presentation and an array presentation of its
     * filters. We get string from db, construct array, and use it until user
     * changes the filter. At the moment of the save, we will commit change.
     * 
     * Now, if commit is successful, update string from array. if commit is
     * failure, update array with string.
     * 
     * This method takes care of the updates after commit.
     */
    public void AfterCommit(boolean success) {
        if (success) {
            this.filterString = Filter.ConstructFilterString(filters);
            originallySubscribed = this.isSubscribed;

        } else {
            filters = Filter.ParseFilterString(this.filterString);
        }
    }

    public static Dataset GetDatasetById(long id) {
        return datasetLookupTable.containsKey(id) ? (Dataset) datasetLookupTable
                .get(id) : null;
    }

    public static Dataset[] GetAllDatasets() {
        Dataset[] all = new Dataset[datasetLookupTable.size()];
        Enumeration<Dataset> e = datasetLookupTable.elements();
        int index = 0;
        while (e.hasMoreElements()) {
            all[index++] = (e.nextElement());
        }
        return all;
    }

    /*
     * folder is to change from partial to subscribed, meaning - filter for this
     * folder becomes null. example input: /F4/F41, F4
     */
    public void IncludeWholeSubfolder(Subfolder folder) {
        Log.d("IncludeWholeSubfolder", "filter:" + this.filterString
                + " to make whole:" + folder.getFolderFullPath());
        String path = folder.getFolderFullPath();
        String top = Filter.getTopFolder(path);

        if (filters != null && filters.length > 0) {
            for (Filter filter : filters) {
                if (filter.mName.equals(top)) {
                    filter.makeWhole(Filter.stripTopFolder(path));
                    return;
                }
            }
        }
    }

    /*
     * Called when nothing under this path is subscribed. Need to remove, if
     * any, specification from the parent node, and cascade upward.
     */
    public void invalidatePath(String path) {
        Log.d("Dataset.invalidatePath", "path:" + path + " original filters:"
                + Filter.ConstructFilterString(filters));
        if (Filter.getDepth(path) == 0) {
            // root level []. Turn this dataset into "unsubscribed"
            if (filters != null && filters.length == 0) {
                setIsSubscribed(false);
            }
            return;
        }

        String top = Filter.getTopFolder(path);

        boolean needsRemove = false;
        for (Filter filter : filters) {
            if (filter.mName.equals(top)) {
                if (Filter.getDepth(path) == 1) {
                    needsRemove = true;
                    break;
                } else {
                    filter.invalidatePath(Filter.stripTopFolder(path));
                    break;
                }
            }
        }

        if (needsRemove) {
            if (filters.length == 1) {
                filters = null;
                this.setIsSubscribed(false);
                return;
            }

            ArrayList<Filter> remainer = new ArrayList<Filter>();
            for (Filter filter : filters) {
                if (!filter.mName.equals(top)) {
                    remainer.add(filter);
                }
            }
            Filter[] updated = new Filter[remainer.size()];
            updated = remainer.toArray(updated);
            this.filters = updated;
        }
        Log.d("Dataset.invalidatePath",
                "updated filters:" + Filter.ConstructFilterString(filters));
    }

    public void nullifyFilter(String path) {
        Log.d("Dataset.nullifyFilter", "path:" + path + " original filters:"
                + Filter.ConstructFilterString(filters));
        if (Filter.getDepth(path) == 0) {
            filters = null;
            filterString = null;
            Log.d("Dataset.nullifyFilter",
                    "updated filters:" + Filter.ConstructFilterString(filters) + " filterString: " + filterString);
            return;
        }

        String top = Filter.getTopFolder(path);

        for (Filter filter : filters) {
            if (filter.mName.equals(top)) {
                if (Filter.getDepth(path) == 1) {
                    filter.mFilters = null;
                    break;
                } else {
                    filter.Nullify(Filter.stripTopFolder(path));
                    break;
                }
            }
        }
        Log.d("Dataset.nullifyFilter",
                "updated filters:" + Filter.ConstructFilterString(filters));
    }

    public void AddFilter(Subfolder folder) {
        Log.d("Dataset.AddFilter", "filter:" + this.filterString + " add:"
                + folder.getFolderFullPath());
        String path = folder.getFolderFullPath();
        String top = Filter.getTopFolder(path);

        if (filters != null && filters.length > 0) {
            for (Filter filter : filters) {
                if (filter.mName.equals(top)) {
                    filter.Add(Filter.stripTopFolder(path));
                    return;
                }
            }
        }

        isSubscribed = true;

        Filter newF = new Filter(top);
        newF.Add(Filter.stripTopFolder(path));

        Filter[] oneMore = new Filter[filters != null ? filters.length + 1 : 1];
        int i = 0;
        if (filters != null && filters.length > 0) {
            for (Filter filter : filters) {
                oneMore[i++] = filter;
            }
        }
        oneMore[i] = newF;
        this.filters = oneMore;

        Log.d("Dataset.AddFilter",
                "filter updated to: " + Filter.ConstructFilterString(filters)
                        + " dataset state:" + this.getSubscriptionState());
    }

    /*
     * Testcases: 1. null, F1-------[{n:F2}]. 2. null,
     * F1/F11---[{n:F1,f:[{n:F12}]},{n:F2}]. 3. [{n:F1}], F1---null. 4.
     * [{n:F1},{n:F2}], F1----[{n:F2}]. 5. [{n:F1}], F1/F11-------[{n:F1,
     * f:[{n:F12}]}]. 6. [{n:F1},{n:F2}], F1/F11----[{n:F1, f:[n:F12]},{n:F2}].
     * 7. [{n:F1,f[{n:F11}]},{n:F2}], F1/F11---[{n:F2}].
     */
    public void RemoveFilter(Subfolder folder) {
        Log.d("Dataset.RemoveFilter",
                "filter:" + Filter.ConstructFilterString(filters) + " remove:"
                        + folder.getFolderFullPath());
        String path = folder.getFolderFullPath();
        String top = Filter.getTopFolder(path);

        boolean needsRemove = false;
        for (Filter filter : filters) {
            if (filter.mName.equals(top)) {
                if (filter.mFilters == null && folder.getPath().equals("/")) {
                    needsRemove = true;
                    break;
                } else {
                    filter.Remove(Filter.stripTopFolder(path));
                    break;
                }
            }
        }
        if (needsRemove) {
            ArrayList<Filter> remainer = new ArrayList<Filter>();
            for (Filter filter : filters) {
                if (!filter.mName.equals(top)) {
                    remainer.add(filter);
                }
            }
            Filter[] updated = new Filter[remainer.size()];
            updated = remainer.toArray(updated);
            this.filters = updated;
        }

        Log.d("Dataset.RemoveFilter",
                "filter updated to: " + Filter.ConstructFilterString(filters));
    }

    /*
     * Subfolder subscription can be dominated by ancestors. e.g. if a parent
     * folder is subscribed, all its children is by default subscribed, and we
     * only need to specify the parent in filter. This method tells if a
     * subfolder is explicitly specified in filter.
     */
    public boolean isExplicitlySpecifiedInFilter(Subfolder folder) {
        String path = folder.getFolderFullPath();
        String top = Filter.getTopFolder(path);
        String tail = Filter.stripTopFolder(path);

        if (filters != null && filters.length > 0) {
            for (Filter filter : filters) {
                if (filter.mName.equals(top)) {
                    if (tail == null || tail.length() == 0) {
                        return true;
                    }
                    return filter.isExplicitlySpecified(tail);
                }
            }
        }

        return false;
    }

    /*
     * Construct filter, if missing, all the way for path. Specify all folders
     * under that path.
     */
    public void specify(SyncListItem[] items) {
        // TODO: this method could be locally sped up a lot by
        // batching up the Add, because folders are siblings.
        for (SyncListItem item : items) {
            // TODO: implement file part!
            if (item instanceof Subfolder) {
                AddFilter((Subfolder) item);
            }
        }
    }

    public Filter getFilterNode(String path) {
        // Log.d(LOG_TAG, "getFilterNode for " + path + " with " +
        // Filter.ConstructFilterString(filters));
        if (filters == null || filters.length == 0) {
            return null;
        }

        String top = Filter.getTopFolder(path);
        for (Filter filter : filters) {
            if (filter.mName.equals(top)) {
                if (Filter.getDepth(path) == 1) {
                    return filter;
                }
                return filter.getNode(Filter.stripTopFolder(path));
            }
        }
        return null;
    }

    public boolean allSubfoldersUnSubscribed(String path) {
        if (Filter.getDepth(path) == 0) {
            for (Filter fi : filters) {
                if (fi.mFilters != null) {
                    return false;
                }
            }
        } else {
            Filter filter = getFilterNode(path);
            return filter.getFilters() != null
                    && filter.getFilters().length == 0;
        }
        return true;
    }
}
