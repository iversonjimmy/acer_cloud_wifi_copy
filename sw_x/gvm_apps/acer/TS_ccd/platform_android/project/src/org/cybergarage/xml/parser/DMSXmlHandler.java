package org.cybergarage.xml.parser;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import org.xml.sax.Attributes;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.DefaultHandler;

import com.acer.ccd.upnp.common.Item;
import com.acer.ccd.upnp.util.Logger;
import com.acer.ccd.upnp.util.UpnpTool;

public class DMSXmlHandler extends DefaultHandler {
    // The tag for LogCat
    private static final String TAG = "DMSXmlHandler";

    private static final int REFERENCED_HD_RESOLUTION = 1920 * 1080;

    // Media Type
    private static final short IMAGE = 1;
    private static final short VIDEO = 2;
    private static final short AUDIO = 3;

    // For UPNP Specification
    private static final short CHILD_COUNT                = 1;
    private static final short COLOR_DEPTH                = 2;
    private static final short CONTAINER                  = 3;
    private static final short DC_CREATOR                 = 4;
    private static final short DC_DATE                    = 5;
    private static final short DC_RIGHTS                  = 6;
    private static final short DC_TITLE                   = 7;
    private static final short DIDL_LITE                  = 8;
    private static final short ID                         = 9;
    private static final short ITEM                       = 10;
    private static final short PARENT_ID                  = 11;
    private static final short PROTOCOL_INFO              = 12;
    private static final short RES                        = 13;
    private static final short RESOLUTION                 = 14;
    private static final short SIZE                       = 15;
    private static final short UPNP_ACTOR                 = 16;
    private static final short UPNP_ALBUM                 = 17;
    private static final short UPNP_ALBUM_ART_URI         = 18;
    private static final short UPNP_ARTIST                = 19;
    private static final short UPNP_AUTHOR                = 20;
    private static final short UPNP_CLASS                 = 21;
    private static final short UPNP_GENRE                 = 22;
    private static final short UPNP_ORIGINAL_TRACK_NUMBER = 23;
    private static final short UPNP_SCHEDULED_START_TIME  = 24;
    private static final short UPNP_SEARCH_CLASS          = 25;
    private static final short UPNP_WRITE_STATUS          = 26;
    private static final short XMLNS                      = 27;
    private static final short XMLNS_DC                   = 28;
    private static final short XMLNS_UPNP                 = 29;

    // For DLNA Specification
    private static final short DLNA_PROFILE_ID            = 30;
    private static final short XMLNS_DLNA                 = 31;

    // For Microsoft Windows Media Player
    private static final short MICROSOFT_CODEC            = 32;
    private static final short XMLNS_MICROSOFT            = 33;

    private static final short BITRATE                    = 34;
    private static final short BITS_PER_SAMPLE            = 35;
    private static final short DESC                       = 36;
    private static final short DURATION                   = 37;
    private static final short INCLUDE_DERIVED            = 38;
    private static final short NAME_SPACE                 = 39;
    private static final short NR_AUDIO_CHANNELS          = 40;
    private static final short REF_ID                     = 41;
    private static final short RESTRICTED                 = 42;
    private static final short ROLE                       = 43;
    private static final short SAMPLE_FREQUENCY           = 44;
    private static final short SEARCHABLE                 = 45;

    // XML Attribute
    private static final String ATTRIBUTE_BITRATE           = "bitrate";
    private static final String ATTRIBUTE_BITS_PER_SAMPLE   = "bitsPerSample";
    private static final String ATTRIBUTE_CHILD_COUNT       = "childCount";
    private static final String ATTRIBUTE_CODEC             = "codec";
    private static final String ATTRIBUTE_COLOR_DEPTH       = "colorDepth";
    private static final String ATTRIBUTE_DC                = "dc";
    private static final String ATTRIBUTE_DLNA              = "dlna";
    private static final String ATTRIBUTE_DLNA_PROFILE_ID   = "dlna:profileID";
    private static final String ATTRIBUTE_DURATION          = "duration";
    private static final String ATTRIBUTE_ID                = "id";
    private static final String ATTRIBUTE_INCLUDE_DERIVED   = "includeDerived";
    private static final String ATTRIBUTE_MICROSOFT         = "microsoft";
    private static final String ATTRIBUTE_MICROSOFT_CODEC   = "microsoft:codec";
    private static final String ATTRIBUTE_NAME_SPACE        = "nameSpace";
    private static final String ATTRIBUTE_NR_AUDIO_CHANNELS = "nrAudioChannels"; 
    private static final String ATTRIBUTE_PARENT_ID         = "parentID";
    private static final String ATTRIBUTE_PROFILE_ID        = "profileID";
    private static final String ATTRIBUTE_PROTOCOL_INFO     = "protocolInfo";
    private static final String ATTRIBUTE_REF_ID            = "refID";
    private static final String ATTRIBUTE_RESOLUTION        = "resolution";
    private static final String ATTRIBUTE_RESTRICTED        = "restricted";
    private static final String ATTRIBUTE_ROLE              = "role";
    private static final String ATTRIBUTE_SAMPLE_FREQUENCY  = "sampleFrequency";
    private static final String ATTRIBUTE_SEARCHABLE        = "searchable";
    private static final String ATTRIBUTE_SIZE              = "size";
    private static final String ATTRIBUTE_UPNP              = "upnp";
    private static final String ATTRIBUTE_XMLNS             = "xmlns";
    private static final String ATTRIBUTE_XMLNS_DC          = "xmlns:dc";
    private static final String ATTRIBUTE_XMLNS_DLNA        = "xmlns:dlna";
    private static final String ATTRIBUTE_XMLNS_MICROSOFT   = "xmlns:microsoft";
    private static final String ATTRIBUTE_XMLNS_UPNP        = "xmlns:upnp";
    
    // XML attributes, object.item.imageItem.photo tag
    private final String JPEG_SM	= "JPEG_SM";
    private final String JPEG_TN	= "JPEG_TN";
    private final String JPEG_MED	= "JPEG_MED";
    private final String JPEG_LRG	= "JPEG_LRG";

    // XML Element
    private static final String ELEMENT_ACTOR                      = "actor";
    private static final String ELEMENT_ALBUM                      = "album";
    private static final String ELEMENT_ALBUM_ART_URI              = "albumArtURI";
    private static final String ELEMENT_ARTIST                     = "artist";
    private static final String ELEMENT_AUTHOR                     = "author";
    private static final String ELEMENT_CLASS                      = "class";
    private static final String ELEMENT_CONTAINER                  = "container";
    private static final String ELEMENT_CREATOR                    = "creator";
    private static final String ELEMENT_DATE                       = "date";
    private static final String ELEMENT_DC_CREATOR                 = "dc:creator";
    private static final String ELEMENT_DC_DATE                    = "dc:date";
    private static final String ELEMENT_DC_RIGHTS                  = "dc:rights";
    private static final String ELEMENT_DC_TITLE                   = "dc:title";
    private static final String ELEMENT_DESC                       = "desc";
    private static final String ELEMENT_DIDL_LITE                  = "DIDL-Lite";
    private static final String ELEMENT_GENRE                      = "genre";
    private static final String ELEMENT_ITEM                       = "item";
    private static final String ELEMENT_ORIGINAL_TRACK_NUMBER      = "originalTrackNumber";
    private static final String ELEMENT_RES                        = "res";
    private static final String ELEMENT_RIGHTS                     = "rights";
    private static final String ELEMENT_SCHEDULED_START_TIME       = "scheduledStartTime";
    private static final String ELEMENT_SEARCH_CLASS               = "searchClass";
    private static final String ELEMENT_TITLE                      = "title";
    private static final String ELEMENT_UPNP_ACTOR                 = "upnp:actor";
    private static final String ELEMENT_UPNP_ALBUM                 = "upnp:album";
    private static final String ELEMENT_UPNP_ALBUM_ART_URI         = "upnp:albumArtURI";
    private static final String ELEMENT_UPNP_ARTIST                = "upnp:artist";
    private static final String ELEMENT_UPNP_AUTHOR                = "upnp:author";
    private static final String ELEMENT_UPNP_CLASS                 = "upnp:class";
    private static final String ELEMENT_UPNP_GENRE                 = "upnp:genre";
    private static final String ELEMENT_UPNP_ORIGINAL_TRACK_NUMBER = "upnp:originalTrackNumber";
    private static final String ELEMENT_UPNP_SCHEDULED_START_TIME  = "upnp:scheduledStartTime";
    private static final String ELEMENT_UPNP_SEARCH_CLASS          = "upnp:searchClass";
    private static final String ELEMENT_UPNP_WRITE_STATUS          = "upnp:writeStatus";
    private static final String ELEMENT_WRITE_STATUS               = "writeStatus";

    private static final HashMap<String, Short> dbFields = new HashMap<String, Short>();

    static {
        dbFields.put( ELEMENT_ACTOR, UPNP_ACTOR );
        dbFields.put( ELEMENT_ALBUM, UPNP_ALBUM );
        dbFields.put( ELEMENT_ALBUM_ART_URI, UPNP_ALBUM_ART_URI );
        dbFields.put( ELEMENT_ARTIST, UPNP_ARTIST );
        dbFields.put( ELEMENT_AUTHOR, UPNP_AUTHOR );
        dbFields.put( ATTRIBUTE_BITRATE, BITRATE );
        dbFields.put( ATTRIBUTE_BITS_PER_SAMPLE, BITS_PER_SAMPLE );
        dbFields.put( ATTRIBUTE_CODEC, MICROSOFT_CODEC );
        dbFields.put( ATTRIBUTE_CHILD_COUNT, CHILD_COUNT );
        dbFields.put( ELEMENT_CLASS, UPNP_CLASS );
        dbFields.put( ATTRIBUTE_COLOR_DEPTH, COLOR_DEPTH );
        dbFields.put( ELEMENT_CONTAINER, CONTAINER );
        dbFields.put( ELEMENT_CREATOR, DC_CREATOR );
        dbFields.put( ELEMENT_DATE, DC_DATE );
        dbFields.put( ATTRIBUTE_DC, XMLNS_DC );
        dbFields.put( ELEMENT_DC_CREATOR, DC_CREATOR );
        dbFields.put( ELEMENT_DC_DATE, DC_DATE );
        dbFields.put( ELEMENT_DC_RIGHTS, DC_RIGHTS );
        dbFields.put( ELEMENT_DC_TITLE, DC_TITLE );
        dbFields.put( ELEMENT_DESC, DESC );
        dbFields.put( ELEMENT_DIDL_LITE, DIDL_LITE );
        dbFields.put( ATTRIBUTE_DLNA, XMLNS_DLNA );
        dbFields.put( ATTRIBUTE_DLNA_PROFILE_ID, DLNA_PROFILE_ID );
        dbFields.put( ATTRIBUTE_DURATION, DURATION );
        dbFields.put( ELEMENT_GENRE, UPNP_GENRE );
        dbFields.put( ATTRIBUTE_ID, ID);
        dbFields.put( ATTRIBUTE_INCLUDE_DERIVED, INCLUDE_DERIVED );
        dbFields.put( ELEMENT_ITEM, ITEM );
        dbFields.put( ATTRIBUTE_MICROSOFT, XMLNS_MICROSOFT );
        dbFields.put( ATTRIBUTE_MICROSOFT_CODEC, MICROSOFT_CODEC );
        dbFields.put( ATTRIBUTE_NAME_SPACE, NAME_SPACE );
        dbFields.put( ATTRIBUTE_NR_AUDIO_CHANNELS, NR_AUDIO_CHANNELS );
        dbFields.put( ELEMENT_ORIGINAL_TRACK_NUMBER, UPNP_ORIGINAL_TRACK_NUMBER );
        dbFields.put( ATTRIBUTE_PARENT_ID, PARENT_ID );
        dbFields.put( ATTRIBUTE_PROFILE_ID, DLNA_PROFILE_ID );
        dbFields.put( ATTRIBUTE_PROTOCOL_INFO, PROTOCOL_INFO );
        dbFields.put( ATTRIBUTE_REF_ID, REF_ID );
        dbFields.put( ELEMENT_RES, RES );
        dbFields.put( ATTRIBUTE_RESOLUTION, RESOLUTION );
        dbFields.put( ATTRIBUTE_RESTRICTED, RESTRICTED );
        dbFields.put( ELEMENT_RIGHTS, DC_RIGHTS );
        dbFields.put( ATTRIBUTE_ROLE, ROLE );
        dbFields.put( ELEMENT_SCHEDULED_START_TIME, UPNP_SCHEDULED_START_TIME );
        dbFields.put( ELEMENT_SEARCH_CLASS, UPNP_SEARCH_CLASS );
        dbFields.put( ATTRIBUTE_SEARCHABLE, SEARCHABLE );
        dbFields.put( ATTRIBUTE_SAMPLE_FREQUENCY, SAMPLE_FREQUENCY );
        dbFields.put( ATTRIBUTE_SIZE, SIZE );
        dbFields.put( ELEMENT_TITLE, DC_TITLE );
        dbFields.put( ATTRIBUTE_UPNP, XMLNS_UPNP );
        dbFields.put( ELEMENT_UPNP_ACTOR, UPNP_ACTOR );
        dbFields.put( ELEMENT_UPNP_ALBUM, UPNP_ALBUM );
        dbFields.put( ELEMENT_UPNP_ALBUM_ART_URI, UPNP_ALBUM_ART_URI );
        dbFields.put( ELEMENT_UPNP_ARTIST, UPNP_ARTIST );
        dbFields.put( ELEMENT_UPNP_AUTHOR, UPNP_AUTHOR );
        dbFields.put( ELEMENT_UPNP_CLASS, UPNP_CLASS );
        dbFields.put( ELEMENT_UPNP_GENRE, UPNP_GENRE );
        dbFields.put( ELEMENT_UPNP_ORIGINAL_TRACK_NUMBER, UPNP_ORIGINAL_TRACK_NUMBER );
        dbFields.put( ELEMENT_UPNP_SCHEDULED_START_TIME, UPNP_SCHEDULED_START_TIME );
        dbFields.put( ELEMENT_UPNP_SEARCH_CLASS, UPNP_SEARCH_CLASS );
        dbFields.put( ELEMENT_UPNP_WRITE_STATUS, UPNP_WRITE_STATUS );
        dbFields.put( ELEMENT_WRITE_STATUS, UPNP_WRITE_STATUS );
        dbFields.put( ATTRIBUTE_XMLNS, XMLNS );
        dbFields.put( ATTRIBUTE_XMLNS_DC, XMLNS_DC );
        dbFields.put( ATTRIBUTE_XMLNS_DLNA, XMLNS_DLNA );
        dbFields.put( ATTRIBUTE_XMLNS_MICROSOFT, XMLNS_MICROSOFT );
        dbFields.put( ATTRIBUTE_XMLNS_UPNP, XMLNS_UPNP );
    }

    // Data Member
    private int mMaxPixels;
    private Item mContent;
    private List<Item> mContentList;

    private boolean isThumbnail;
    private boolean isValid;
    private boolean passCharacters;
    private String profileId;
    private String role;
    private StringBuilder currentText;
    
    private boolean ImageItemProcessing;
    private final int UNDEFINE 		= -1;
    private final int RES_TAG 		= 0;
    private final int IMG_URL 		= 1;
    private final int RES_VALUES 	= 2;
    private int	res_index;
    private String[][] resolution_set;
    private final String[][] resolution_set_init = {{JPEG_TN, null, null},
                                                    {JPEG_SM, null, null},
                                                    {JPEG_MED, null, null},
                                                    {JPEG_LRG, null, null}};

	    private short detectMediaType ( String mediaInfo ) {
        short ret = 0;

        if ( null != mediaInfo && "" != mediaInfo ) {
            if ( true == mediaInfo.contains( "image" )) {
                ret = IMAGE;
            } else if (true == mediaInfo.contains("video")) {
                ret = VIDEO;
            } else if (true == mediaInfo.contains("audio")) {
                ret = AUDIO;
            }
        }

        return ret;
    }

    private short mapToField( String tag ) {
        short ret = 0;

        if (null != tag && "" != tag) {
            if (dbFields.containsKey(tag)) {
                ret = dbFields.get(tag);
            }
        }

        return ret; 
    }
    
    private int findTAGIndex(String Token){
    	for(int i=0; i<resolution_set.length; i++){
    		if( Token.contains(resolution_set[i][RES_TAG]) ){
    			//Logger.d(TAG," res = " + resolution_set[i][RES_TAG]);
    			return i;    			
    		}
    	}
    	return UNDEFINE;
    }

    public DMSXmlHandler() {
        mContent = null;
        mContentList = null;
        profileId = null;
        role = null;

        mMaxPixels = 0;
        currentText = new StringBuilder();
    }

    @Override
    public void characters( char[] ch, int start, int length )
            throws SAXException {
    	super.characters(ch, start, length);
    	//Logger.d(TAG, "      characters " + length);
        if ( false == passCharacters ) {
            if (0 < length) {
                if ('\n' != ch[0]) {
                    currentText.append(ch, start, length);
                }
            }
        }
    }


    @Override
    public void startDocument() throws SAXException {
    	super.startDocument();
        mContentList = new ArrayList<Item>();
        Logger.w(TAG, "startDocument()");
    }
	
	    @Override
    public void endDocument() throws SAXException {
	    	super.endDocument();
	    Logger.w(TAG, "endDocument()");
        // Do Nothing;
    }

	
	
	
	
	
    @Override
    public void endElement( String uri, String localName, String qName ) throws SAXException {
		super.endElement(uri, localName, qName);
        String name = localName;

        //Logger.d(TAG, "    end Element " + uri + "," + localName + "," + qName);
        
        switch ( mapToField( name ) ) {
            case CONTAINER:
                mContentList.add( mContent );
                break;

            case DC_CREATOR:
                mContent.setCreator( currentText.toString());
                break;

            case DC_DATE:
                mContent.setDate( currentText.toString());
                break;

            case DC_TITLE:
                mContent.setTitle( currentText.toString());
                break;

            case ITEM:
                if (ImageItemProcessing) {
                    // Desc search from TN to LRG
                    for (int i = 0; i < resolution_set.length; i++) {
                        String URL = resolution_set[i][IMG_URL];
                        if (null != URL) {
                            // Logger.d(TAG, " set thumbnail:" +
                            // resolution_set[i][RES_TAG]);
                            mContent.setThumbnailUrl(URL);
                            break;
                        }
                    }

                    // ASC search from LRG to TN
                    for (int i = resolution_set.length - 1; i >= 0; i--) {
                        String URL = resolution_set[i][IMG_URL];
                        if (null != URL) {
                            // Logger.d(TAG, " set url:" +
                            // resolution_set[i][RES_TAG]);
                            mContent.setUrl(URL);
                            mContent.setResolution(resolution_set[i][RES_VALUES]);
                            break;
                        }
                    }
            	}
            	ImageItemProcessing = false;
                mContentList.add( mContent );
                break;

            case RES:            	
            	if(res_index != UNDEFINE){
            		resolution_set[res_index][IMG_URL] = currentText.toString();
            		//Logger.d(TAG, " res_index " + res_index);
            	}
                break;

            case UPNP_ACTOR:
                mContent.setActor( currentText.toString());
                break;

            case UPNP_ALBUM:
                mContent.setAlbum( currentText.toString());
                break;

            case UPNP_ALBUM_ART_URI:
                mContent.setAlbumUrl( currentText.toString());
                profileId = null;
                break;

            case UPNP_ARTIST:
                if ( true == isValid && null != role ) {
                    if ( true == role.equals( "AlbumArtist" )) {
                        mContent.setArtist( currentText.toString());
                    }
                    role = null;
                }

            
                break;

            case UPNP_CLASS:
                mContent.setUpnpClass( currentText.toString());
                break;

            case UPNP_GENRE:
                mContent.setGenre( currentText.toString());
                break;

            default:
            	Logger.d(TAG, "   SKIP !!");
                // Do Nothing;
                break;
        }

        currentText.setLength( 0 );
    }

	
	
	
	
	
    @Override
    public void startElement( String uri, String localName, String qName, Attributes attributes )
            throws SAXException {
    	super.startElement(uri, localName, qName, attributes);
    	//Logger.d(TAG, "_   start Element " + uri + ", " + localName + ", " + qName);
    	 
        isValid = true;
        isThumbnail = false;
        passCharacters = false;
        String name = localName;        

        switch ( mapToField( name ) ) {
            case CONTAINER:
                mContent = new Item();
                mContent.setId( attributes.getValue( ATTRIBUTE_ID ));
                mContent.setParentid( attributes.getValue( ATTRIBUTE_PARENT_ID ));
                mContent.setChildCount( attributes.getValue( ATTRIBUTE_CHILD_COUNT ));
                break;

            case ITEM:
                mContent = new Item();
                mContent.setId( attributes.getValue( ATTRIBUTE_ID ));
                mContent.setParentid( attributes.getValue( ATTRIBUTE_PARENT_ID ));
                mMaxPixels = 0x7FFFFFFF;                

                ImageItemProcessing = false;
                resolution_set = null;
                resolution_set = resolution_set_init;
                break;

            case RES: {
                ImageItemProcessing = true;
                    String protocolInfo = attributes.getValue( ATTRIBUTE_PROTOCOL_INFO );
                    String resolution = null;

                    if ( null != protocolInfo ) {
                        String protocolInfoToken[] = protocolInfo.split( ":" );
                        short iva = detectMediaType( protocolInfoToken[2] );

                        switch ( iva ) {
                            case IMAGE:
                            	res_index = UNDEFINE;
                            	
                            if (true == protocolInfoToken[2].endsWith("x-ycbcr-yuv420")) {
                                isValid = false;
                                passCharacters = true;
                            } else {
                                res_index = findTAGIndex(protocolInfoToken[3]);

                                resolution = attributes.getValue(ATTRIBUTE_RESOLUTION);
                                    if ( (UNDEFINE != res_index) && null != resolution ) {
                                    	resolution_set[res_index][RES_VALUES] = resolution;
                                        int difference = 0;
                                        int pixels = 0;
                                        String[] hw = resolution.split( "x" );

                                        pixels = Integer.parseInt( hw[0] ) * Integer.parseInt( hw[1] );
                                        difference = Math.abs( REFERENCED_HD_RESOLUTION - pixels );

                                    if (mMaxPixels > difference) {
                                        mMaxPixels = difference;
                                    } else {
                                        isValid = false;
                                        passCharacters = true;
                                    }

                                    hw = null;
                                } else if (null != mContent.getUrl()) {
                                    isValid = false;
                                    passCharacters = true;
                                }
                                }
                                break;

                            case VIDEO:
                                if ( null == mContent.getUrl()) {
                                    /*if ( true == protocolInfoToken[0].equals( "rtsp-rtp-udp" )) { 
                                        isValid = false;
                                        passCharacters = true;
                                    }
                                    else if ( true == protocolInfoToken[3].contains( "DLNA.ORG_CI=1" )) {
                                        isValid = false;
                                        passCharacters= true;
                                    }
                                    else if ( true == protocolInfoToken[2].endsWith( "x-ms-wmv" )) {
                                        if ( false == protocolInfoToken[3].contains( "DLNA.ORG_PN=WMVSPML_BASE" ) && 
                                             false == protocolInfoToken[3].contains( "DLNA.ORG_PN=WMVSPLL_BASE" )) {
                                            isValid = false;
                                            passCharacters = true;
                                        }
                                    }*/
                                    if ( false == protocolInfoToken[0].equals( "rtsp-rtp-udp" ) &&
                                         false == protocolInfoToken[3].contains("DLNA.ORG_CI=1")) {
                                    resolution = attributes.getValue(ATTRIBUTE_RESOLUTION);
                                } else {
                                    isValid = false;
                                    passCharacters = true;
                                }
                                }
                                break;

                            case AUDIO:
                                if ( true == protocolInfoToken[3].contains( "DLNA.ORG_CI=1" )) {
                                    isValid = false;
                                    passCharacters = true;
                                }
                                break;

                            default:
                                isValid = false;
                                passCharacters = true;
                                break;
                        }

                        //if ( true == isValid && false == isThumbnail ) {
                        if ( true == isValid ) {

                            String size = attributes.getValue( ATTRIBUTE_SIZE );

                        if (IMAGE == iva || VIDEO == iva) {
                            if (null == resolution) {
                                resolution = "";
                            }
                            mContent.setResolution(resolution);
                                resolution = null;
                            }

                            if ( AUDIO == iva || VIDEO == iva ) {
                                mContent.setDuration( UpnpTool.getTimeSecond( attributes.getValue( ATTRIBUTE_DURATION )));    
                            }

                        if (null == size) {
                            size = "0";
                        }

                            mContent.setFileSize( size );
                            mContent.setFormat( protocolInfoToken[2] );
                            mContent.setProtocolInfo( protocolInfo );
                            mContent.setProtocolName( protocolInfoToken[0] );

                            size = null;
                        }

                        protocolInfo = null;
                        protocolInfoToken = null;
                    }
                }
                break;

            case UPNP_ALBUM_ART_URI:
                profileId = attributes.getValue( ATTRIBUTE_PROFILE_ID );
                
                if ( false == profileId.equals( "JPEG_TN" )) {
                    isValid = false;
                    passCharacters = true;
                }

                break;

            case UPNP_ARTIST:
            	role = attributes.getValue( ATTRIBUTE_ROLE );
                if (role != null) {
                    if (false == role.equals("AlbumArtist")) {
                        isValid = false;
                        passCharacters = true;
                    }
                } else {
                    isValid = false;
            		passCharacters = true;
            	}
                break;

            case DC_CREATOR:
            case DC_DATE:
            case DC_TITLE:            
            case UPNP_ACTOR:
            case UPNP_ALBUM:
            case UPNP_CLASS:
            case UPNP_GENRE:
            case UPNP_ORIGINAL_TRACK_NUMBER:
                break;

            default:
                isValid = false;
                passCharacters = true;
                Logger.d(TAG,"   skip Element");
                break;
        }
    }

    public List<Item> getItemList() {
        return mContentList;
    }
}
