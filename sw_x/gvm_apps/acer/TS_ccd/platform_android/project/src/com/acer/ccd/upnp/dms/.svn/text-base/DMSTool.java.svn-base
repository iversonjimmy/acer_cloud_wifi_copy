/******************************************************************
 *
 *	Clear.fi for Android
 *
 *	Copyright (C) Acer
 *
 * 	R&D Chaozhong Li
 * 
 *	File: DMSInfo.java
 *
 *	Revision:
 *
 *	2011-3-9
 *		- first revision.
 *	
 ******************************************************************/

package com.acer.ccd.upnp.dms;

import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.UnsupportedEncodingException;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.parsers.SAXParser;
import javax.xml.parsers.SAXParserFactory;

import org.cybergarage.upnp.Argument;
import org.cybergarage.upnp.Device;
import org.cybergarage.xml.parser.DMSXmlHandler;
import org.w3c.dom.Document;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;
import org.xml.sax.XMLReader;

import com.acer.ccd.cache.data.DlnaAudio;
import com.acer.ccd.cache.data.DlnaContainer;
import com.acer.ccd.cache.data.DlnaImage;
import com.acer.ccd.cache.data.DlnaSearchResult;
import com.acer.ccd.cache.data.DlnaVideo;
import com.acer.ccd.upnp.common.Item;
import com.acer.ccd.upnp.util.Logger;
import com.acer.ccd.upnp.util.Upnp;
import com.acer.ccd.upnp.util.UpnpTool;

/**
 * The Class DMS.
 * 
 * @author chaozhong li
 */
public class DMSTool {

    /** The Constant tag. */
    private final static String TAG = "DMSTool";

    /** The Constant tag. */
    private final static String defaultvalue = null;// "unkown";

    /**
     * Parses the content result.
     * 
     * @param dev the dev
     * @param result the result
     * @return the list
     */
    public static int count = 0;
    // Flag to determine to use SAXParser or DOM.
    public static final boolean IS_USING_SAX_PARSER = true;
    
    private static void createFile(byte[] bytes) {
        try {
            String s = new String(bytes, "UTF-8");
            File f = new File("/sdcard/clear.fi/" + (count++) + ".xml");
            try {
                FileWriter fw = new FileWriter(f);
                fw.write(s);
                fw.flush();
                fw.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        } catch (UnsupportedEncodingException e) {
            e.printStackTrace();
        }

    }

    public static List<Item> parseContentResult(Device dev, Argument result) {
        if (null == dev || null == result) {
            return null;
        }

        List<Item> list = new ArrayList<Item>();

        if (IS_USING_SAX_PARSER) {
            DMSXmlHandler xmlHandler = new DMSXmlHandler();
            try {
                SAXParserFactory spf = SAXParserFactory.newInstance();
                SAXParser sp = spf.newSAXParser();

                /* Get the XMLReader of the SAXParser we created. */
                XMLReader xr = sp.getXMLReader();
                /* Create a new ContentHandler and apply it to the XML-Reader*/
                xr.setContentHandler(xmlHandler);
                
                /* Parse the xml-data from our URL. */
                Logger.i(TAG, "result = " + result.getValue());
                
                xr.parse(new InputSource(new ByteArrayInputStream(result.getValue().getBytes("UTF-8"))));
            } catch (Exception e) {
            	Logger.e(TAG, " XML parsing error:" + e.getMessage());
            }
            return xmlHandler.getItemList();
        }
        
        DocumentBuilderFactory dfactory = DocumentBuilderFactory.newInstance();
        DocumentBuilder documentBuilder;
        try {
            documentBuilder = dfactory.newDocumentBuilder();
            InputStream is = new ByteArrayInputStream(result.getValue().getBytes("UTF-8"));
            Logger.e(TAG, String.valueOf(result.getValue().getBytes()));
            Document doc = documentBuilder.parse(is);

            String deviceName = dev.getFriendlyName();
            String deviceUuid = dev.getUDN();

            String objectClass = defaultvalue;
            String id = defaultvalue;
            String parentid = defaultvalue;
            String album = defaultvalue;
            String artist = defaultvalue;
            String creator = defaultvalue;
            String dateTaken = defaultvalue;
            String description = defaultvalue;
            String duration = defaultvalue;
            String fileSize = defaultvalue;
            String format = defaultvalue;
            String genre = defaultvalue;
            String albumArtURI = defaultvalue;
            String publisher = defaultvalue;
            String recordingYear = defaultvalue;
            String title = defaultvalue;
            String trackNo = defaultvalue;
            String url = defaultvalue;
            String thumbnailUrl = defaultvalue;
            String coordinate = defaultvalue;
            String resolution = defaultvalue;
            long maxResolution = 0;
            long minResolution = 0;
            String actor = defaultvalue;
            String bitRate = defaultvalue;
            String codec = defaultvalue;
            String protocolName = defaultvalue;
            String childCount = defaultvalue;
            String protocolInfo = defaultvalue;

            // parse container
            NodeList containers = doc
                    .getElementsByTagName(Upnp.ContentProp.Object.container.CONTENT_PROP_CONTAINER);
            for (int j = 0; j < containers.getLength(); ++j) {
                objectClass = defaultvalue;
                id = defaultvalue;
                parentid = defaultvalue;
                title = defaultvalue;
                childCount = defaultvalue;

                Node container = containers.item(j);
                NodeList childNodes = container.getChildNodes();
                for (int icnt = 0; icnt < childNodes.getLength(); ++icnt) {
                    Node childNode = childNodes.item(icnt);

                    if (childNode.getNodeName().equals(Upnp.ContentProp.dc.CONTENT_PROP_TITLE)) {
                        title = childNode.getFirstChild().getNodeValue();
                        id = container.getAttributes().getNamedItem(Upnp.ContentProp.didllite.CONTENT_PROP_ID)
                                .getNodeValue();
                        parentid = container.getAttributes().getNamedItem(
                                Upnp.ContentProp.didllite.CONTENT_PROP_PARENT_ID).getNodeValue();
                    } else if (childNode.getNodeName().equals(Upnp.ContentProp.upnp.CONTENT_PROP_CLASS_TAG)) {
                        objectClass = childNode.getFirstChild().getNodeValue();
                    } else if (childNode.getNodeName().equals(Upnp.ContentProp.didllite.CONTENT_PROP_CHILD_COUNT)) {
                        childCount = childNode.getFirstChild().getNodeValue();
                    }
                }
                Item i = new Item();
                i.setDeviceName(deviceName);
                i.setDeviceUuid(deviceUuid);

                i.setObjectClass(objectClass);
                i.setId(id);
                i.setParentid(parentid);
                i.setTitle(title);
                i.setChildCount(childCount);

                list.add(i);

                Logger.v(TAG, i.toString());
            }

            // parse item
            NodeList items = doc.getElementsByTagName(Upnp.ContentProp.Object.item.CONTENT_PROP_ITEM);
            for (int j = 0; j < items.getLength(); ++j) {
                objectClass = defaultvalue;
                id = defaultvalue;
                parentid = defaultvalue;

                album = defaultvalue;
                artist = defaultvalue;
                creator = defaultvalue;
                dateTaken = defaultvalue;
                description = defaultvalue;

                duration = defaultvalue;
                fileSize = defaultvalue;
                format = defaultvalue;
                genre = defaultvalue;
                albumArtURI = defaultvalue;
                publisher = defaultvalue;
                recordingYear = defaultvalue;
                title = defaultvalue;
                trackNo = defaultvalue;
                url = defaultvalue;
                thumbnailUrl = defaultvalue;
                coordinate = defaultvalue;
                resolution = defaultvalue;
                maxResolution = 0;
                minResolution = 0;
                actor = defaultvalue;
                bitRate = defaultvalue;
                codec = defaultvalue;
                protocolName = defaultvalue;
                childCount = defaultvalue;
                protocolInfo = defaultvalue;

                Node item = items.item(j);
                id = item.getAttributes().getNamedItem(Upnp.ContentProp.didllite.CONTENT_PROP_ID).getNodeValue();
                parentid = item.getAttributes().getNamedItem(Upnp.ContentProp.didllite.CONTENT_PROP_PARENT_ID)
                        .getNodeValue();
                NodeList childNodes = item.getChildNodes();
                for (int l = 0; l < childNodes.getLength(); ++l) {
                    Node childNode = childNodes.item(l);

                    if (childNode.getNodeName().equals(Upnp.ContentProp.upnp.CONTENT_PROP_ALBUM)) {
                        album = childNode.getFirstChild().getNodeValue();
                    } else if (childNode.getNodeName().equals(Upnp.ContentProp.upnp.CONTENT_PROP_ARTIST)) {
                        artist = childNode.getFirstChild().getNodeValue();
                    } else if (childNode.getNodeName().equals(Upnp.ContentProp.dc.CONTENT_PROP_CREATOR)) {
                        creator = childNode.getFirstChild().getNodeValue();
                    } else if (childNode.getNodeName().equals(Upnp.ContentProp.dc.CONTENT_PROP_DATE)) {
                        dateTaken = childNode.getFirstChild().getNodeValue();
                    } else if (childNode.getNodeName().equals(Upnp.ContentProp.dc.CONTENT_PROP_DESCRIPTION)) {
                        description = childNode.getFirstChild().getNodeValue();
                    } else if (childNode.getNodeName().equals(Upnp.ContentProp.upnp.CONTENT_PROP_GENRE)) {
                        genre = childNode.getFirstChild().getNodeValue();
                    } else if (childNode.getNodeName().equals(Upnp.ContentProp.dc.CONTENT_PROP_PUBLISHER)) {
                        publisher = childNode.getFirstChild().getNodeValue();
                    } else if (childNode.getNodeName().equals(Upnp.ContentProp.dc.CONTENT_PROP_TITLE)) {
                        title = childNode.getFirstChild().getNodeValue();
                    } else if (childNode.getNodeName().equals(Upnp.ContentProp.upnp.CONTENT_PROP_ACTOR)) {
                        actor = childNode.getFirstChild().getNodeValue();
                    } else if (childNode.getNodeName().equals(Upnp.ContentProp.upnp.CONTENT_PROP_CLASS_TAG)) {
                        objectClass = childNode.getFirstChild().getNodeValue();
                    } else if (childNode.getNodeName().equals(Upnp.ContentProp.upnp.CONTENT_PROP_ALBUM_ART_URI)) {
                        albumArtURI = childNode.getFirstChild().getNodeValue();
                    } else if (childNode.getNodeName().equals(
                            Upnp.ContentProp.upnp.CONTENT_PROP_ORIGINAL_TRACK_NUMBER)) {
                        trackNo = childNode.getFirstChild().getNodeValue();
                    } else if (childNode.getNodeName().equals(Upnp.ContentProp.didllite.CONTENT_PROP_CHILD_COUNT)) {
                        childCount = childNode.getFirstChild().getNodeValue();
                    } else if (childNode.getNodeName().equals(Upnp.ContentProp.didllite.res.CONTENT_PROP_RES)) {
                        String urlTmp = childNode.getFirstChild().getNodeValue();
                        // check ip address
                        if (!checkUrl(TAG, urlTmp)) {
                            Logger.w(TAG, "error" + url);
                            continue;
                        }
                        String protocolInfoTmp = "";
                        String protocolNameTmp = "";
                        String formatTmp = "";
                        if (childNode.getAttributes().getNamedItem(
                                Upnp.ContentProp.didllite.res.CONTENT_PROP_PROTOCOL_INFO) != null) {
                            protocolInfoTmp = childNode.getAttributes().getNamedItem(
                                    Upnp.ContentProp.didllite.res.CONTENT_PROP_PROTOCOL_INFO).getNodeValue();
                            String[] protocolInfoRst = protocolInfoTmp.split(":");
                            protocolNameTmp = protocolInfoRst[0];
                            formatTmp = protocolInfoRst[2];
                        }
                        // "x-ycbcr-yuv420"
                        // check resolution size
                        if (childNode.getAttributes().getNamedItem(
                                Upnp.ContentProp.didllite.res.CONTENT_PROP_RESOLUTION) != null) {
                            resolution = childNode.getAttributes().getNamedItem(
                                    Upnp.ContentProp.didllite.res.CONTENT_PROP_RESOLUTION).getNodeValue();
                            String[] resol = resolution.split("x");
                            if (resol.length > 1) {
                                try {
                                    long resolutionsize = Integer.parseInt(resol[0])
                                            * Integer.parseInt(resol[1]);

                                    if (resolutionsize > 0) {
                                        if (resolutionsize == maxResolution) {
                                            if (0 == minResolution) {
                                                minResolution = maxResolution;
                                            }
                                            if (formatTmp.contains("x-ycbcr-yuv420")) {
                                                continue;
                                            }
                                            url = urlTmp;
                                        } else if (resolutionsize > maxResolution) {
                                            maxResolution = resolutionsize;
                                            if (0 == minResolution) {
                                                minResolution = maxResolution;
                                            }
                                            url = urlTmp;
                                        } else if (resolutionsize < minResolution
                                                && !formatTmp.contains("x-ycbcr-yuv420")) {
                                            minResolution = resolutionsize;
                                            thumbnailUrl = urlTmp;
                                            continue;
                                        } else {
                                            continue;
                                        }
                                    } else {
                                        continue;
                                    }
                                } catch (NullPointerException e) {
                                    e.printStackTrace();
                                    continue;
                                }
                            } else {
                                url = urlTmp;
                                thumbnailUrl = null;
                            }
                        } else {
                            url = urlTmp;
                            thumbnailUrl = null;
                        }

                        if (childNode.getAttributes().getNamedItem(
                                Upnp.ContentProp.didllite.res.CONTENT_PROP_PROTOCOL_INFO) != null) {
                            protocolInfo = childNode.getAttributes().getNamedItem(
                                    Upnp.ContentProp.didllite.res.CONTENT_PROP_PROTOCOL_INFO).getNodeValue();
                            String[] protocolInfoRst = protocolInfo.split(":");
                            for (int i = 0; i < protocolInfoRst.length; ++i) {
                                Logger.v(TAG, "protocolInfoRst[" + i + "]: " + protocolInfoRst[i]);
                            }
                            protocolName = protocolInfoRst[0];
                            format = protocolInfoRst[2];
                        }
                        if (childNode.getAttributes().getNamedItem(
                                Upnp.ContentProp.didllite.res.CONTENT_PROP_DURATION) != null) {
                            duration = childNode.getAttributes().getNamedItem(
                                    Upnp.ContentProp.didllite.res.CONTENT_PROP_DURATION).getNodeValue();
                        }
                        if (childNode.getAttributes().getNamedItem(
                                Upnp.ContentProp.didllite.res.CONTENT_PROP_SIZE) != null) {
                            fileSize = childNode.getAttributes().getNamedItem(
                                    Upnp.ContentProp.didllite.res.CONTENT_PROP_SIZE).getNodeValue();
                        }
                        if (childNode.getAttributes().getNamedItem(
                                Upnp.ContentProp.didllite.res.CONTENT_PROP_BITRATE) != null) {
                            bitRate = childNode.getAttributes().getNamedItem(
                                    Upnp.ContentProp.didllite.res.CONTENT_PROP_BITRATE).getNodeValue();
                        }

                    }
                }
                Item i = new Item();
                i.setDeviceName(deviceName);
                i.setDeviceUuid(deviceUuid);

                if (Upnp.ContentProp.upnp.classProperty.OBJECT_ITEM_AUDIOITEM_MUSICTRACK
                        .equals(objectClass)
                        || Upnp.ContentProp.upnp.classProperty.OBJECT_ITEM_VIDEOITEM
                                .equals(objectClass)
                        || Upnp.ContentProp.upnp.classProperty.OBJECT_ITEM_IMAGEITEM
                                .equals(objectClass)) {
                    i.setDuration(UpnpTool.getTimeSecond(duration));
                }
                i.setObjectClass(objectClass);
                i.setId(id);
                i.setParentid(parentid);
                i.setAlbum(album);
                i.setArtist(artist);
                i.setCreator(creator);
                i.setDateTaken(dateTaken);
                i.setDescription(description);
                i.setDuration(UpnpTool.getTimeSecond(duration));
                i.setFileSize(fileSize);
                i.setFormat(format);
                i.setGenre(genre);
                i.setAlbumUrl(albumArtURI);
                i.setPublisher(publisher);
                i.setRecordingYear(recordingYear);
                i.setTitle(title);
                i.setTrackNo(trackNo);
                Logger.i(TAG, "set url:" + url);
                i.setUrl(url);
                Logger.i(TAG, "set ThumbnailUrl:" + thumbnailUrl);
                i.setThumbnailUrl(thumbnailUrl);
                i.setCoordinate(coordinate);
                i.setResolution(resolution);
                i.setActor(actor);
                i.setBitRate(bitRate);
                i.setCodec(codec);
                i.setProtocolName(protocolName);
                i.setChildCount(childCount);
                i.setProtocolInfo(protocolInfo);
                list.add(i);
                Logger.v(TAG, i.toString());
            }
        } catch (ParserConfigurationException e) {
            e.printStackTrace();
        } catch (UnsupportedEncodingException e) {
            e.printStackTrace();
        } catch (SAXException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
        return list;
    }

    /**
     * Prints the search result.
     * 
     * @param tag the tag
     * @param tag2
     * @param dlnaSearchResult the dlna search result
     */
    public static void printSearchResult(String tag, String tag2, DlnaSearchResult dlnaSearchResult) {
        if (null == dlnaSearchResult) {
            return;
        }

        //Logger.v(tag, tag2 + " SearchResult getArtist:" + dlnaSearchResult.getArtist());
        //Logger.v(tag, tag2 + " SearchResult getDeviceName:" + dlnaSearchResult.getDeviceName());
        //Logger.v(tag, tag2 + " SearchResult getDeviceUuid:" + dlnaSearchResult.getDeviceUuid());
        //Logger.v(tag, tag2 + " SearchResult getAlbumUrl:" + dlnaSearchResult.getAlbumUrl());
        //Logger.v(tag, tag2 + " SearchResult getMusic:" + dlnaSearchResult.getMusic());
        //Logger.v(tag, tag2 + " SearchResult getMusicAlbum:" + dlnaSearchResult.getMusicAlbum());
        //Logger.v(tag, tag2 + " SearchResult getMusicDuration:"
        //        + dlnaSearchResult.getMusicDuration());
        //Logger.v(tag, tag2 + " SearchResult getParentContainerId:"
        //        + dlnaSearchResult.getParentContainerId());
        //Logger.v(tag, tag2 + " SearchResult getPhoto:" + dlnaSearchResult.getPhoto());
        //Logger.v(tag, tag2 + " SearchResult getPhotoAlbum:" + dlnaSearchResult.getPhotoAlbum());
        //Logger.v(tag, tag2 + " SearchResult getUrl:" + dlnaSearchResult.getUrl());
        //Logger.v(tag, tag2 + " SearchResult getVideo:" + dlnaSearchResult.getVideo());
        //Logger.v(tag, tag2 + " SearchResult getVideoAlbum:" + dlnaSearchResult.getVideoAlbum());

    }

    /**
     * Prints the item.
     * 
     * @param tag the tag
     * @param item the item
     */
    public static void printItem(String tag, Item item) {
        if (null == item) {
            return;
        }
        //Logger.v(tag, "Server Item getActor    :" + item.getActor());
        //Logger.v(tag, "Server Item getAlbum    :" + item.getAlbum());
        //Logger.v(tag, "Server Item getArtist   :" + item.getArtist());
        //Logger.v(tag, "Server Item getBitRate  :" + item.getBitRate());
        //Logger.v(tag, "Server Item getChildCount   :" + item.getChildCount());
        //Logger.v(tag, "Server Item getCodec    :" + item.getCodec());
        //Logger.v(tag, "Server Item getCoordinate   :" + item.getCoordinate());
        //Logger.v(tag, "Server Item getCreator  :" + item.getCreator());
        //Logger.v(tag, "Server Item getDateTaken    :" + item.getDateTaken());
        //Logger.v(tag, "Server Item getDescription  :" + item.getDescription());
        //Logger.v(tag, "Server Item getDeviceName   :" + item.getDeviceName());
        //Logger.v(tag, "Server Item getDeviceUuid   :" + item.getDeviceUuid());
        //Logger.v(tag, "Server Item getDuration :" + item.getDuration());
        //Logger.v(tag, "Server Item getFileSize :" + item.getFileSize());
        //Logger.v(tag, "Server Item getFormat   :" + item.getFormat());
        //Logger.v(tag, "Server Item getGenre    :" + item.getGenre());
        //Logger.v(tag, "Server Item getAlbumUrl :" + item.getAlbumUrl());
        //Logger.v(tag, "Server Item getId   :" + item.getId());
        //Logger.v(tag, "Server Item getParentid   :" + item.getParentid());
        //Logger.v(tag, "Server Item getObjectClass  :" + item.getObjectClass());
        //Logger.v(tag, "Server Item getProtocolName :" + item.getProtocolName());
        //Logger.v(tag, "Server Item getPublisher    :" + item.getPublisher());
        //Logger.v(tag, "Server Item getRecordingYear    :" + item.getRecordingYear());
        //Logger.v(tag, "Server Item getResolution   :" + item.getResolution());
        //Logger.v(tag, "Server Item getTitle    :" + item.getTitle());
        //Logger.v(tag, "Server Item getTrackNo  :" + item.getTrackNo());
        //Logger.v(tag, "Server Item getUrl  :" + item.getUrl());
        //Logger.v(tag, "Server Item getThumbnailUrl  :" + item.getThumbnailUrl());
    }

    /**
     * Prints the db audio item.
     * 
     * @param tag the tag
     * @param dlnaAudio the dlna audio
     */
    public static void printDBAudioItem(String tag, DlnaAudio dlnaAudio) {
        if (null == dlnaAudio) {
            return;
        }
        //Logger.v(tag, "DB Audio getAlbum:" + dlnaAudio.getAlbum());
        //Logger.v(tag, "DB Audio getArtist:" + dlnaAudio.getArtist());
        //Logger.v(tag, "DB Audio getCreator:" + dlnaAudio.getCreator());
        //Logger.v(tag, "DB Audio getDateTaken:" + dlnaAudio.getDateTaken());
        //Logger.v(tag, "DB Audio getDescription:" + dlnaAudio.getDescription());
        //Logger.v(tag, "DB Audio getDeviceName:" + dlnaAudio.getDeviceName());
        //Logger.v(tag, "DB Audio getDeviceUuid:" + dlnaAudio.getDeviceUuid());
        //Logger.v(tag, "DB Audio getDuration:" + dlnaAudio.getDuration());
        //Logger.v(tag, "DB Audio getFileSize:" + dlnaAudio.getFileSize());
        //Logger.v(tag, "DB Audio getFormat:" + dlnaAudio.getFormat());
        //Logger.v(tag, "DB Audio getGenre:" + dlnaAudio.getGenre());
        //Logger.v(tag, "DB Audio getAlbumUrl:" + dlnaAudio.getAlbumUrl());
        //Logger.v(tag, "DB Audio getProtocolName:" + dlnaAudio.getProtocolName());
        //Logger.v(tag, "DB Audio getPublisher:" + dlnaAudio.getPublisher());
        //Logger.v(tag, "DB Audio getTitle:" + dlnaAudio.getTitle());
        //Logger.v(tag, "DB Audio getTrackNo:" + dlnaAudio.getTrackNo());
        //Logger.v(tag, "DB Audio getUrl:" + dlnaAudio.getUrl());
    }

    /**
     * Prints the db video item.
     * 
     * @param tag the tag
     * @param dlnaVideo the dlna video
     */
    public static void printDBVideoItem(String tag, DlnaVideo dlnaVideo) {
        if (null == dlnaVideo) {
            return;
        }
        //Logger.v(tag, "DB Video getActor:" + dlnaVideo.getActor());
        //Logger.v(tag, "DB Video getAlbum:" + dlnaVideo.getAlbum());
        //Logger.v(tag, "DB Video getArtist:" + dlnaVideo.getArtist());
        //Logger.v(tag, "DB Video getBitRate:" + dlnaVideo.getBitRate());
        //Logger.v(tag, "DB Video getCreator:" + dlnaVideo.getCreator());
        //Logger.v(tag, "DB Video getDateTaken:" + dlnaVideo.getDateTaken());
        //Logger.v(tag, "DB Video getDescription:" + dlnaVideo.getDescription());
        //Logger.v(tag, "DB Video getDeviceName:" + dlnaVideo.getDeviceName());
        //Logger.v(tag, "DB Video getDuration:" + dlnaVideo.getDuration());
        //Logger.v(tag, "DB Video getFileSize:" + dlnaVideo.getFileSize());
        //Logger.v(tag, "DB Video getFormat:" + dlnaVideo.getFormat());
        //Logger.v(tag, "DB Video getGenre:" + dlnaVideo.getGenre());
        //Logger.v(tag, "DB Video getAlbumUrl:" + dlnaVideo.getAlbumUrl());
        //Logger.v(tag, "DB Audio getProtocolName:" + dlnaVideo.getProtocolName());
        //Logger.v(tag, "DB Video getPublisher" + dlnaVideo.getPublisher());
        //Logger.v(tag, "DB Video getResolution:" + dlnaVideo.getResolution());
        //Logger.v(tag, "DB Video getTitle" + dlnaVideo.getTitle());
        //Logger.v(tag, "DB Video getUrl:" + dlnaVideo.getUrl());
    }

    /**
     * Prints the db image item.
     * 
     * @param tag the tag
     * @param dlnaImage the dlna image
     */
    public static void printDBImageItem(String tag, DlnaImage dlnaImage) {
        if (null == dlnaImage) {
            return;
        }
        //Logger.v(tag, "DB Image getAlbum:" + dlnaImage.getAlbum());
        //Logger.v(tag, "DB Image getCreator:" + dlnaImage.getCreator());
        //Logger.v(tag, "DB Image getDateTaken:" + dlnaImage.getDateTaken());
        //Logger.v(tag, "DB Image getDescription:" + dlnaImage.getDescription());
        //Logger.v(tag, "DB Image getDeviceName:" + dlnaImage.getDeviceName());
        //Logger.v(tag, "DB Image getFileSize:" + dlnaImage.getFileSize());
        //Logger.v(tag, "DB Image getFormat:" + dlnaImage.getFormat());
        //Logger.v(tag, "DB Image getAlbumUrl:" + dlnaImage.getAlbumUrl());
        //Logger.v(tag, "DB Audio getProtocolName:" + dlnaImage.getProtocolName());
        //Logger.v(tag, "DB Image getPublisher:" + dlnaImage.getPublisher());
        //Logger.v(tag, "DB Image getResolution:" + dlnaImage.getResolution());
        //Logger.v(tag, "DB Image getTitle:" + dlnaImage.getTitle());
        //Logger.v(tag, "DB Image getUrl:" + dlnaImage.getUrl());
        //Logger.v(tag, "DB Image getThumbnailUrl:" + dlnaImage.getThumbnailUrl());
    }

    /**
     * Prints the db container.
     * 
     * @param tag the tag
     * @param dlnaContainer the dlna container
     */
    public static void printDBContainer(String tag, DlnaContainer dlnaContainer) {
        if (null == dlnaContainer) {
            return;
        }
        Logger.v(tag, "DB Container getContainerId:" + dlnaContainer.getContainerId());
        Logger.v(tag, "DB Container getTitle:" + dlnaContainer.getTitle());
    }

    /**
     * Check url.
     * 
     * @param tag the tag
     * @param url the url
     * @return true, if successful
     */
    public static boolean checkUrl(String tag, String url) {
        if (null == url || !url.startsWith("http://")) {
            return false;
        }

        String httphead = "http://";
        String[] urlip = url.substring(httphead.length()).split("\\.");

        try {
            Enumeration<NetworkInterface> nis = NetworkInterface.getNetworkInterfaces();
            while (nis.hasMoreElements()) {
                NetworkInterface ni = (NetworkInterface)nis.nextElement();
                Enumeration<InetAddress> addrs = ni.getInetAddresses();
                while (addrs.hasMoreElements()) {
                    InetAddress addr = (InetAddress)addrs.nextElement();
                    String ipAddress = addr.getHostAddress();
                    Logger.v(tag, "ip address:" + ipAddress);
                    if (null != ipAddress && !ipAddress.contains("127.0.0.1")) {
                        String[] ip = ipAddress.split("\\.");

                        if (ip[0].equalsIgnoreCase(urlip[0]) && ip[1].equalsIgnoreCase(urlip[1])) {
                            Logger.v(tag, "true");
                            return true;
                        }
                    }
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }

        return false;
    }

}
