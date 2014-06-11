package com.acer.ccd.upnp.dmr;

import java.util.ArrayList;

public class DmrActionList extends ArrayList{
    public DmrActionList(){
        
    }
    
    public DMRAction getDmrAction(int n)
    {
        return (DMRAction)get(n);
    }
    
    public DMRAction getDmrAction(String dmrUuid){
        DMRAction dmrAction = null;
        for(int i = 0; i < size(); i ++){
            dmrAction = getDmrAction(i);
            if(dmrAction.getDmrUuid().equalsIgnoreCase(dmrUuid)){
                return dmrAction;
            }
        }
        
        return null;
    }
    
    public void release(){
        DMRAction dmrAction = null;
        for(int i = 0; i < size(); i ++){
            dmrAction = getDmrAction(i);
            dmrAction.release();
        }
    }
    
}
