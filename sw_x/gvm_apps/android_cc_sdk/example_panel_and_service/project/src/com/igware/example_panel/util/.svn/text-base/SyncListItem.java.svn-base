package com.igware.example_panel.util;

/**
 * @author cindy
 *
 */
public abstract class SyncListItem implements Comparable<SyncListItem>{ 
    public enum State {
        SUBSCRIBED, PARTIALLY_SUBSCRIBED, UNSUBSCIRBED;
       }
    
    protected String name;    

    public String getName() {
        return name;
    }   

    public abstract State getSubscriptionState(); 
    
    // these three methods intend only to change the object 
    // in memory to represent the actual subscription structure.
    
    // from unsubscribed state
    public abstract void subscribe();
    
    // from subscribed state
    public abstract void unsubscribe();
    
    // from partial state to subscribed
    public abstract void partialToAll();
   
    /**
     * @see java.lang.Object#toString()
     */
    @Override
    public String toString() {
        return name;
    }
    
    /**
     * @param aThat is a non-null Dataset.
     *
     * @throws NullPointerException if aThat is null.
     */
     public int compareTo( SyncListItem aThat ) {       
       final int EQUAL = 0;       
       
       if ( this == aThat ) return EQUAL;       
       
       return this.name.compareTo(aThat.name);
     }
}
