package com.acer.ccd.cache.data;

/**
 * Container item data
 */
public class ContainerItem {
private Object mIndex;
private boolean mIsContainer;

    public ContainerItem(boolean isByPosition)
    {
        mIsContainer = false;
        if (!isByPosition)
        {
            setId(new Long(-1));
        }
        else
        {
            setId(new Integer(-1));
        }
    }

    public boolean isContainer() {
        return mIsContainer;
    }

    public void setContainer(boolean isContainer) {
        this.mIsContainer = isContainer;
    }

    public void setId(Object id) {
        this.mIndex = id;
    }

    public Object getId() {
        return mIndex;
    }
}

