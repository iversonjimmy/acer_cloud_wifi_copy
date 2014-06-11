package com.acer.dx.util.igware;

public class Subfolder extends SyncListItem {

    public enum Type {
        Folder,
        File
    }
    protected String path;
    protected Dataset dataset; // The dataset it belongs to
    protected Type type;

    protected long modifyTime;
    protected long size;
    protected String url;

    public Dataset getDataset() {
        return dataset;
    }

    public String getPath() {
        return path;
    }
    
    public Type getType() {
        return type;
    }

    public long getModifyTime() {
        return modifyTime;
    }

    public long getSize() {
        return size;
    }

    public String getUrl() {
        return url;
    }

    public void setDataset(Dataset dataset) {
        this.dataset = dataset;
    }

    public void setPath(String path) {
        this.path = path;
    }

    @Override
    public State getSubscriptionState() {
        return dataset.getSubScriptionStateForSubFolder(path, this.name);
    }

    public Subfolder(String n, String p, Type t) {
        name = n;
        path = p;
        type = t;
    }

    public Subfolder(String n, String p, Type t, long mTime, long s, String u) {
        name = n;
        path = p;
        type = t;
        modifyTime = mTime;
        size = s;
        url = u;
    }
    public String getAbsoluteFolderDevicePath() {
        String absDevicePath = this.dataset.getAbsoluteDeviceRoot();
        String[] s1 = new String[]{};
        if (absDevicePath!=null) { 
            s1 = absDevicePath.split("/");
        }
        String fileFullPath = getFolderFullPath();
        String[] s2 =  fileFullPath.split("/");
        
        StringBuilder sb = new StringBuilder();
        for(String s: s1) {
            sb.append(s);
            sb.append("/");
        }        
        for(String s: s2) {
            sb.append(s);            
            sb.append("/");
        }
        String sbString = sb.toString();
        return sbString.substring(0, sbString.length()-1);
    }
    
    public String getFolderFullPath() {
        // TODO: implement animity
        if (path.equals("/")) {
            return path + name;
        }
        return path.endsWith("/") ? path + name : path + "/" + name;
    }

    @Override
    public void subscribe() {
        dataset.AddFilter(this);
    }

    @Override
    public void unsubscribe() {
        dataset.RemoveFilter(this);
    }

    @Override
    public void partialToAll() {
        dataset.IncludeWholeSubfolder(this);
    }
}
