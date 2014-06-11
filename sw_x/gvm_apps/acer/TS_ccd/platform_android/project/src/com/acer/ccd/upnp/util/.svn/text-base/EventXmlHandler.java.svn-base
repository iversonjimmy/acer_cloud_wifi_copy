package com.acer.ccd.upnp.util;

import java.util.ArrayList;
import java.util.List;

import org.xml.sax.Attributes;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.DefaultHandler;


public class EventXmlHandler extends DefaultHandler{
	private static final String TAG = "EventXmlHandler";
    private List<EventItem> itemList;
    private EventItem item;
    private static final String TRANSPROT_STATE = "TransportState";
    private static final String EVENT = "Event";
    private static final String CURRENT_TRACK = "CurrentTrack";
   
    private boolean isTransportState = false;
    private boolean isEvent = false;
    private boolean isCurrentTrack = false;
    private static final boolean DEBUG = false;
    
    public EventXmlHandler(){
    }
    
    @Override
    public void startDocument() throws SAXException {
        super.startDocument();
        itemList = new ArrayList<EventItem>();
    }
    
    private boolean isItemNeeded() {
        return (isTransportState | isCurrentTrack);
    }
    
    
    @Override
    public void characters(char[] ch, int start, int length)
            throws SAXException {
        super.characters(ch, start, length);
        
        if (!isItemNeeded())
            return;
        String s = new String(ch, start, length);
        
        if (isTransportState){
            item.setTransportState(s);
        }
        if(isCurrentTrack){
            item.setCurrentTrack(s);
        }
    }
    
    
    @Override
    public void endDocument() throws SAXException {
        super.endDocument();
    }
    
    @Override
    public void endElement(String uri, String localName, String qName)
            throws SAXException {
        super.endElement(uri, localName, qName);
        if(DEBUG){
            Logger.i(TAG, "end uri = " + uri);
            Logger.i(TAG, "end localName = " + localName);
            Logger.i(TAG, "end qName = " + qName);
        }
        
        
        
        
        if (localName.equals(TRANSPROT_STATE)){
            isTransportState = false;
        }
        else if(localName.equals(CURRENT_TRACK)){
            isCurrentTrack = false;
        }
        else if(localName.equals(EVENT)){
            isEvent = false;
            itemList.add(item);
        }
    }
    
    @Override
    public void startElement(String uri, String localName, String qName,
            Attributes attr) throws SAXException {
        super.startElement(uri, localName, qName, attr);
        if(DEBUG){
            Logger.i(TAG, "start uri = " + uri);
            Logger.i(TAG, "start localName = " + localName);
            Logger.i(TAG, "start qName = " + qName);
            Logger.i(TAG, "start attr = " + attr.getValue(0));
        }
        
        
        
        
        if (localName.equals(TRANSPROT_STATE)){
            isTransportState = true;
            item.setTransportState(attr.getValue(0));
        }
        if(localName.equals(CURRENT_TRACK)){
            isCurrentTrack = true;
            item.setCurrentTrack(attr.getValue(0));
        }
        else if(localName.equals(EVENT)){
            item = null;
            item = new EventItem();
            isEvent = true;
        }
        
    }
    
    
    public List<EventItem> getItemList() {
        return itemList;
    }
}

