package com.igware.example_panel.util;

import java.util.ArrayList;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import com.igware.example_panel.util.SyncListItem.State;

import android.util.Log;

/**
 * 
 * @author cindy
 * 
 */
public class Filter {
    String mName;
    Filter[] mFilters;
    private static final String LOG_TAG = "Filter";

    public Filter[] getFilters() {
        return mFilters;
    }

    public String getName() {
        return mName;
    }

    public Filter(JSONObject jsonObj) {
        if (jsonObj == null) {
            return;
        }
        try {
            String s = jsonObj.getString("n");
            if (s != null && s.length() > 0) {
                mName = s;
            }
            if (jsonObj.has("f")) {
                JSONArray filterArray = jsonObj.getJSONArray("f");
                if (filterArray != null) {
                    Filter[] filters = new Filter[filterArray.length()];
                    for (int i = 0; i < filterArray.length(); i++) {
                        JSONObject obj = (JSONObject) filterArray.get(i);
                        filters[i] = new Filter(obj);
                    }
                    this.mFilters = filters;
                }
            }

        } catch (Exception ex) {
            Log.e(LOG_TAG, ex.getMessage());
        }
    }

    public Filter(String n) {
        mName = n;
    }

    private JSONObject getJSONObject() throws JSONException {
        if (mName == null && mFilters == null) {
            return null;
        }
        JSONObject o = new JSONObject();
        o.put("n", mName);
        JSONArray array = new JSONArray();
        if (mFilters != null) {
            for (Filter item : mFilters) {
                array.put(item.getJSONObject());
            }
            o.put("f", array);
        }
        return o;
    }

    /*
     * Takes the filterstring from db, spit out array of filters.
     */
    public static Filter[] ParseFilterString(String filterstring) {
        try {
            if (filterstring != null) {// && filterstring.trim().length() > 0) {
                JSONArray jsonArray = new JSONArray(filterstring);
                Filter[] filters = new Filter[jsonArray.length()];
                for (int i = 0; i < jsonArray.length(); i++) {
                    JSONObject obj = jsonArray.getJSONObject(i);
                    filters[i] = new Filter(obj);
                }
                return filters;
            }

        } catch (Exception e) {

        }
        return null;
    }

    public static String ConstructFilterString(Filter[] filters) {
        if (filters == null) {
            return "";
        }

        JSONArray array = new JSONArray();

        try {
            for (Filter item : filters) {
                JSONObject obj = item.getJSONObject();
                if (obj != null) {
                    array.put(item.getJSONObject());
                }
            }
        } catch (Exception ex) {

        }
        return array.toString();
    }

    public boolean isExplicitlySpecified(String path) {
        int depth = Filter.getDepth(path);
        if (depth == 0) {
            return true;
        }

        Filter dest = null;
        if (depth == 1) {
            String top = getTopFolder(path);
            dest = new Filter(top);

            if (mFilters == null || mFilters.length == 0) {
                return false;
            } else {
                for (Filter filter : mFilters) {
                    if (filter.mName.equals(top)) {
                        return true;
                    }
                }
                // TODO: this is a really bad error case.
                // It mean we're trying to unsubscribe a subfolder which
                // according to
                // filter is NOT subscribed already. Must investigate when
                // happens.
                Log.e(LOG_TAG, "Should not happen! filter: " + this.toString()
                        + " path:" + path);
                return false;
            }
        } else {
            // recurse
            String topFolder = getTopFolder(path);
            if (mFilters != null) {
                for (Filter filter : mFilters) {
                    if (filter.getName().equals(topFolder)) {
                        dest = filter;
                    }
                }
            }
            if (dest != null) {
                return dest.isExplicitlySpecified(stripTopFolder(path));
            } else {
                return false;
            }
        }
    }

    /*
     * Get the filter node, make it contain full .filters
     */
    public void Nullify(String path) {
        Filter filter = getNode(path);
        filter.mFilters = null;
    }
    
    /*
     * Take the filter node specified by path out of its parent node
     */
    public void invalidatePath(String path) {
        Filter filter = getNode(Filter.stripTailFolder(path));
	if (filter==null) {
	    return;
	}

        ArrayList<Filter> remainer = new ArrayList<Filter>();
        String tail = Filter.getTailFolder(path);
        
        for (Filter fi : filter.mFilters) {
            if (!fi.mName.equals(tail)) {
                remainer.add(filter);
            }
        }
        Filter[] updated = new Filter[remainer.size()];
        updated = remainer.toArray(updated);
        filter.mFilters = updated;
    }

    public Filter getNode(String path) {
        int depth = Filter.getDepth(path);

        if (depth == 0) {
	    return this;
        } else if (depth == 1) {
            if (mFilters == null) {
                return null;
            } else {
                for (Filter filter : mFilters) {
                    if (filter.mName.equals(getTopFolder(path))) {
                        return filter;

                    }
                }
            }
        } else {
            String head = getTopFolder(path);
            String tail = stripTopFolder(path);
            if (mFilters == null) {
                Log.e(LOG_TAG, "Should not happen! filter: " + this.toString()
                        + " path:" + path);
            } else {
                for (Filter filter : mFilters) {
                    if (filter.mName.equals(head)) {
                        return filter.getNode(tail);
                    }
                }
            }
        }
        return null;
    }

    public void makeWhole(String path) {
        int depth = Filter.getDepth(path);
        if (depth == 0) {
            mFilters = null;
        } else if (depth == 1) {
            assert mFilters != null && mFilters.length > 0;

            for (Filter filter : mFilters) {
                if (filter.mName.equals(Filter.getTailFolder(path))) {
                    filter.mFilters = null;
                }
            }
        } else {
            // recurse
            assert mFilters != null && mFilters.length > 0;
            String topFolder = getTopFolder(path);

            Filter dest = null;
            for (Filter filter : mFilters) {
                if (filter.getName().equals(topFolder)) {
                    dest = filter;
                }
            }

            assert dest != null;
            dest.makeWhole(stripTopFolder(path));
        }
    }

    /*
     * path should be a relative path starting from current filter's position in
     * the tree
     */
    public void Add(String path) {
        int depth = getDepth(path);
        if (depth == 0) {
            return;
        }

        Filter dest = null;
        String top = getTopFolder(path);
        if (depth == 1) {// flat filter at current level
            dest = new Filter(top);

            if (mFilters == null || mFilters.length == 0) {
                mFilters = new Filter[] { dest };
            } else {
                Filter[] oneMore = new Filter[mFilters.length + 1];
                int i = 0;
                for (Filter filter : mFilters) {
                    oneMore[i++] = filter;
                }
                oneMore[i] = dest;
                mFilters = oneMore;
            }
        } else {
            // recurse
            String topFolder = getTopFolder(path);
            if (mFilters != null) {
                for (Filter filter : mFilters) {
                    if (filter.getName().equals(topFolder)) {
                        dest = filter;
                    }
                }
            }
            if (dest != null) {
                dest.Add(stripTopFolder(path));
            } else {
                dest = new Filter(topFolder);
                dest.Add(stripTopFolder(path));
                int newLength = mFilters == null ? 1 : mFilters.length + 1;
                Filter[] oneMore = new Filter[newLength];
                int i = 0;
                if (mFilters != null) {
                    for (Filter filter : mFilters) {
                        oneMore[i++] = filter;
                    }
                }
                oneMore[i] = dest;
                mFilters = oneMore;
            }
        }
    }

    public void Remove(String path) {
        int depth = getDepth(path);
        if (depth == 0) {
            Log.e(LOG_TAG, "Should not happen! filter: " + this.toString()
                    + " path:" + path);
            return;
        }

        if (depth == 1) {// flat filter at current level
            if (mFilters == null) {
                Log.e(LOG_TAG, "Should not happen! null filter");
            } else {
                boolean needsRemove = false;
                String top = getTopFolder(path);

                for (Filter filter : mFilters) {
                    if (filter.mName.equals(top)) {
                        needsRemove = true;
                        break;
                    }
                }

                if (needsRemove) {
                    ArrayList<Filter> remainer = new ArrayList<Filter>();
                    for (Filter filter : mFilters) {
                        if (!filter.mName.equals(top)) {
                            remainer.add(filter);
                        }
                    }
                    Filter[] updated = new Filter[remainer.size()];
                    updated = remainer.toArray(updated);
                    this.mFilters = updated;
                }
            }
        } else {
            String head = getTopFolder(path);
            String tail = stripTopFolder(path);
            if (mFilters == null) {
                Log.e(LOG_TAG, "Should not happen! filter: " + this.toString()
                        + " path:" + path);
            } else {
                for (Filter filter : mFilters) {
                    if (filter.mName.equals(head)) {
                        filter.Remove(tail);
                        break;
                    }
                }
            }
        }
    }

    /*
     * example: a/b/c : a a: a
     */
    public static String getTopFolder(String path) {
        int depth = getDepth(path);
        if (depth == 0) {
            return null;
        }

        String[] folders = path.split("/");
        for (String folder : folders) {
            if (folder != null && folder.trim().length() > 0) {
                return folder;
            }
        }
        return null;
    }

    public static String getTailFolder(String path) {
        int depth = getDepth(path);
        if (depth == 0) {
            return null;
        }

        String[] folders = path.split("/");
        for (int i = folders.length - 1; i >= 0; i--) {
            if (folders[i] != null && folders[i].trim().length() > 0) {
                return folders[i];
            }
        }
        return null;
    }

    public static String stripTopFolder(String path) {
        int depth = getDepth(path);
        if (depth < 2) {
            return null;
        }

        String[] folders = path.split("/");

        StringBuilder sb = new StringBuilder();
        boolean skipFirst = false;
        for (String folder : folders) {
            if (folder != null && folder.trim().length() > 0) {
                if (!skipFirst) {
                    skipFirst = true;
                    continue;
                } else {
                    sb.append(folder);
                    sb.append("/");
                }
            }
        }
        return sb.toString();
    }

    public static String stripTailFolder(String path) {
        int depth = getDepth(path);
        if (depth < 2) {
            return null;
        }

        String[] folders = path.split("/");
        StringBuilder sb = new StringBuilder();

        int count = 0;
        for (int i = 0; i < folders.length && count < depth - 1; i++) {
            if (folders[i] != null && folders[i].trim().length() > 0) {
                sb.append(folders[i]);
                sb.append("/");
                count++;
            }
        }
        return sb.toString();
    }

    public static String getFirstN(String path, int n) {
        if (n == 0) {
            return null;
        }
        int depth = getDepth(path);
        if (depth <= n) {
            return path;
        }
        String[] folders = path.split("/");
        StringBuilder sb = new StringBuilder();

        int count = 0;
        for (int i = 0; i < folders.length && count < n; i++) {
            if (folders[i] != null && folders[i].trim().length() > 0) {
                sb.append(folders[i]);
                sb.append("/");
                count++;
            }
        }

        return sb.toString();
    }

    public static int getDepth(String path) {
        if (path == null || path.length() == 0) {
            return 0;
        }
        String[] folders = path.split("/");
        int depth = 0;
        for (int i = 0; i < folders.length; i++) {
            if (folders[i] != null && folders[i].trim().length() > 0) {
                depth++;
            }
        }
        return depth;
    }

    /**
     * @see java.lang.Object#toString()
     */
    @Override
    public String toString() {
        if ((mName == null || mName.equals(""))
                && (mFilters == null || mFilters.length == 0)) {
            return null;
        }
        try {
            JSONObject o = new JSONObject();
            o.put("n", mName);
            o.put("f", mFilters);
            return o.toString();
        } catch (Exception e) {

        }
        return null;
    }

    /*
     * path must start from current loc in filesystem.
     */
    public State isVisible(String path, String folderName) {
        String top = getTopFolder(path);
        if (top != null && top.trim().length() > 0) {
            if (top.equals(mName)) {
                if (mFilters != null) {
                    for (Filter filter : mFilters) {
                        State t = filter.isVisible(stripTopFolder(path),
                                folderName);
                        if (t != State.UNSUBSCIRBED) {
                            return t;
                        }
                    }
                } else {
                    return State.SUBSCRIBED;
                }
            }
        } else {
            if (folderName.equals(mName)) {
                if (mFilters == null || mFilters.length == 0) {
                    return State.SUBSCRIBED;
                }
                return State.PARTIALLY_SUBSCRIBED;
            }
        }
        return State.UNSUBSCIRBED;
    }
}