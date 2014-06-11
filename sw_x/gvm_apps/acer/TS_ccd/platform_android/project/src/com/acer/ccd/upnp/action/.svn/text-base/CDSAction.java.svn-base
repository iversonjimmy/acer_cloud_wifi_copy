/******************************************************************
 *
 *	Clear.fi for Android
 *
 *	Copyright (C) Acer
 *
 * 	R&D Chaozhong Li
 * 
 *	File: CDSAction.java
 *
 *	Revision:
 *
 *	2011-3-10
 *		- first revision.
 *	
 ******************************************************************/

package com.acer.ccd.upnp.action;

import org.cybergarage.upnp.Action;
import org.cybergarage.upnp.ArgumentList;
import org.cybergarage.upnp.Device;
import org.cybergarage.upnp.UPnPStatus;

import com.acer.ccd.upnp.util.Logger;
import com.acer.ccd.upnp.util.Upnp;
import com.acer.ccd.upnp.util.UpnpTool;

// TODO: Auto-generated Javadoc
/**
 * The Class CDSAction.
 * 
 * @author acer
 */
public class CDSAction extends BaseAction {

    /** The tag. */
    private final static String tag = "CDSAction";

    /**
     * Instantiates a new cDS action.
     * 
     * @param dev the dev
     */
    public CDSAction(Device dev) {
        super(dev);
        // TODO Auto-generated constructor stub
    }

    /**
     * Browse.
     * 
     * @param ObjectID the object id
     * @param BrowseFlag the browse flag
     * @param Filter the filter
     * @param StartingIndex the starting index
     * @param RequestedCount the requested count
     * @param SortCriteria the sort criteria
     * @return the argument list
     */
    public ArgumentList browse(String ObjectID, String BrowseFlag, String Filter,
            String StartingIndex, String RequestedCount, String SortCriteria) {
        Logger.v(tag, "browse(" + ObjectID + "," + BrowseFlag + "," + Filter + "," + StartingIndex
                + "," + RequestedCount + "," + SortCriteria + ")");
        org.cybergarage.upnp.Service service = mDevice.getService(Upnp.Service.SERVICE_CDS1);
        if (null == service) {
            return null;
        }

        Action action = service.getAction(Upnp.CDSAction.CDS_ACTION_BROWSE);
        if (null == action) {
            return null;
        }
        action.lock();
        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.CDSArgVariable.Browse.CDS_VARIABLE_IN_OBJECT_ID).setValue(ObjectID);
        argumentList.getArgument(Upnp.CDSArgVariable.Browse.CDS_VARIABLE_IN_BROWSE_FLAG).setValue(BrowseFlag);
        argumentList.getArgument(Upnp.CDSArgVariable.Browse.CDS_VARIABLE_IN_STARTING_INDEX)
                .setValue(StartingIndex);
        argumentList.getArgument(Upnp.CDSArgVariable.Browse.CDS_VARIABLE_IN_REQUESTED_COUNT).setValue(
                RequestedCount);
        argumentList.getArgument(Upnp.CDSArgVariable.Browse.CDS_VARIABLE_IN_FILTER).setValue(Filter);
        argumentList.getArgument(Upnp.CDSArgVariable.Browse.CDS_VARIABLE_IN_SORT_CRITERIA).setValue(SortCriteria);

        if ( action.postControlAction() ) {
            ArgumentList outArgList = action.getOutputArgumentList();
            UpnpTool.printArgContent(Upnp.CDSAction.CDS_ACTION_BROWSE, outArgList,
                    Upnp.CDSArgVariable.Browse.CDS_VARIABLE_OUT_RESULT);
            UpnpTool.printArgContent(Upnp.CDSAction.CDS_ACTION_BROWSE, outArgList,
                    Upnp.CDSArgVariable.Browse.CDS_VARIABLE_OUT_NUMBER_RETURNED);
            UpnpTool.printArgContent(Upnp.CDSAction.CDS_ACTION_BROWSE, outArgList,
                    Upnp.CDSArgVariable.Browse.CDS_VARIABLE_OUT_TOTAL_MATCHES);
            UpnpTool.printArgContent(Upnp.CDSAction.CDS_ACTION_BROWSE, outArgList,
                    Upnp.CDSArgVariable.Browse.CDS_VARIABLE_OUT_UPDATE_ID);
            action.unlock();
            return outArgList;
        } else {
            UPnPStatus err = action.getControlStatus();
            Logger.e(tag, "Error Code = " + err.getCode());
            Logger.e(tag, "Error Desc = " + err.getDescription());
            action.unlock();
            return null;
        }
    }

    /**
     * Creates the object.
     * 
     * @param ContainerID the container id
     * @param Elements the elements
     * @return the argument list
     */
    public ArgumentList createObject(String ContainerID, String Elements) {
        Logger.v(tag, "createObject(" + ContainerID + "," + Elements + ")");
        org.cybergarage.upnp.Service service = mDevice.getService(Upnp.Service.SERVICE_CDS1);
        if (null == service) {
            return null;
        }

        Action action = service.getAction(Upnp.CDSAction.CDS_ACTION_CREATE_OBJECT);
        if (null == action) {
            return null;
        }
        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.CDSArgVariable.CreateObject.CDS_VARIABLE_IN_CONTAINER_ID).setValue(
                ContainerID);
        argumentList.getArgument(Upnp.CDSArgVariable.CreateObject.CDS_VARIABLE_IN_ELEMENTS).setValue(Elements);

        if (action.postControlAction()) {
            ArgumentList outArgList = action.getOutputArgumentList();
            UpnpTool.printArgContent(Upnp.CDSAction.CDS_ACTION_CREATE_OBJECT, outArgList,
                    Upnp.CDSArgVariable.CreateObject.CDS_VARIABLE_OUT_OBJECT_ID);
            UpnpTool.printArgContent(Upnp.CDSAction.CDS_ACTION_CREATE_OBJECT, outArgList,
                    Upnp.CDSArgVariable.CreateObject.CDS_VARIABLE_OUT_RESULT);
            return outArgList;
        } else {
            UPnPStatus err = action.getControlStatus();
            Logger.e(tag, "Error Code = " + err.getCode());
            Logger.e(tag, "Error Desc = " + err.getDescription());
            return null;
        }
    }

    /**
     * Creates the reference.
     * 
     * @param ContainerID the container id
     * @param ObjectID the object id
     * @return the argument list
     */
    public ArgumentList createReference(String ContainerID, String ObjectID) {
        Logger.v(tag, "createReference(" + ContainerID + "," + ObjectID + ")");
        org.cybergarage.upnp.Service service = mDevice.getService(Upnp.Service.SERVICE_CDS1);
        if (null == service) {
            return null;
        }

        Action action = service.getAction(Upnp.CDSAction.CDS_ACTION_CREATE_REFERENCE);
        if (null == action) {
            return null;
        }
        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.CDSArgVariable.CreateReference.CDS_VARIABLE_IN_CONTAINER_ID).setValue(
                ContainerID);
        argumentList.getArgument(Upnp.CDSArgVariable.CreateReference.CDS_VARIABLE_IN_OBJECT_ID).setValue(ObjectID);

        if (action.postControlAction()) {
            ArgumentList outArgList = action.getOutputArgumentList();
            UpnpTool.printArgContent(Upnp.CDSAction.CDS_ACTION_CREATE_REFERENCE, outArgList,
                    Upnp.CDSArgVariable.CreateReference.CDS_VARIABLE_OUT_NEW_ID);
            return outArgList;
        } else {
            UPnPStatus err = action.getControlStatus();
            Logger.e(tag, "Error Code = " + err.getCode());
            Logger.e(tag, "Error Desc = " + err.getDescription());
            return null;
        }
    }

    /**
     * Destroy object.
     * 
     * @param ObjectID the object id
     * @return true, if successful
     */
    public boolean DestroyObject(String ObjectID) {
        Logger.v(tag, "DestroyObject(" + ObjectID + ")");
        org.cybergarage.upnp.Service service = mDevice.getService(Upnp.Service.SERVICE_CDS1);
        if (null == service) {
            return false;
        }

        Action action = service.getAction(Upnp.CDSAction.CDS_ACTION_DESTROY_OBJECT);
        if (null == action) {
            return false;
        }
        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.CDSArgVariable.DestroyObject.CDS_VARIABLE_IN_OBJECT_ID).setValue(ObjectID);

        if (action.postControlAction()) {
            return true;
        } else {
            UPnPStatus err = action.getControlStatus();
            Logger.e(tag, "Error Code = " + err.getCode());
            Logger.e(tag, "Error Desc = " + err.getDescription());
            return false;
        }
    }

    /**
     * Export resource.
     * 
     * @param SourceURI the source uri
     * @param DestinationURI the destination uri
     * @return the argument list
     */
    public ArgumentList exportResource(String SourceURI, String DestinationURI) {
        Logger.v(tag, "exportResource(" + SourceURI + "," + DestinationURI + ")");
        org.cybergarage.upnp.Service service = mDevice.getService(Upnp.Service.SERVICE_CDS1);
        if (null == service) {
            return null;
        }

        Action action = service.getAction(Upnp.CDSAction.CDS_ACTION_EXPORT_RESOURCE);
        if (null == action) {
            return null;
        }
        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.CDSArgVariable.ExportResource.CDS_VARIABLE_IN_SOURCE_URI)
                .setValue(SourceURI);
        argumentList.getArgument(Upnp.CDSArgVariable.ExportResource.CDS_VARIABLE_IN_DESTINATION_URI).setValue(
                DestinationURI);

        if (action.postControlAction()) {
            ArgumentList outArgList = action.getOutputArgumentList();
            UpnpTool.printArgContent(Upnp.CDSAction.CDS_ACTION_EXPORT_RESOURCE, outArgList,
                    Upnp.CDSArgVariable.ExportResource.CDS_VARIABLE_OUT_TRANSFER_ID);
            return outArgList;
        } else {
            UPnPStatus err = action.getControlStatus();
            Logger.e(tag, "Error Code = " + err.getCode());
            Logger.e(tag, "Error Desc = " + err.getDescription());
            return null;
        }
    }

    /**
     * Delete resource.
     * 
     * @param ResourceURI the resource uri
     * @return true, if successful
     */
    public boolean DeleteResource(String ResourceURI) {
        Logger.v(tag, "DeleteResource(" + ResourceURI + ")");
        org.cybergarage.upnp.Service service = mDevice.getService(Upnp.Service.SERVICE_CDS1);
        if (null == service) {
            return false;
        }

        Action action = service.getAction(Upnp.CDSAction.CDS_ACTION_DELETE_RESOURCE);
        if (null == action) {
            return false;
        }
        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.CDSArgVariable.DeleteResource.CDS_VARIABLE_IN_RESOURCE_URI).setValue(
                ResourceURI);
        if (action.postControlAction()) {
            return true;
        } else {
            UPnPStatus err = action.getControlStatus();
            Logger.e(tag, "Error Code = " + err.getCode());
            Logger.e(tag, "Error Desc = " + err.getDescription());
            return false;
        }
    }

    /**
     * Gets the feature list.
     * 
     * @return the feature list
     */
    public ArgumentList getFeatureList() {
        Logger.v(tag, "getFeatureList( )");
        org.cybergarage.upnp.Service service = mDevice.getService(Upnp.Service.SERVICE_CDS1);
        if (null == service) {
            return null;
        }

        Action action = service.getAction(Upnp.CDSAction.CDS_ACTION_GET_FEATURE_LIST);
        if (null == action) {
            return null;
        }

        if (action.postControlAction()) {
            ArgumentList outArgList = action.getOutputArgumentList();
            UpnpTool.printArgContent(Upnp.CDSAction.CDS_ACTION_GET_FEATURE_LIST, outArgList,
                    Upnp.CDSArgVariable.GetFeatureList.CDS_VARIABLE_OUT_FEATURE_LIST);
            return outArgList;
        } else {
            UPnPStatus err = action.getControlStatus();
            Logger.e(tag, "Error Code = " + err.getCode());
            Logger.e(tag, "Error Desc = " + err.getDescription());
            return null;
        }
    }

    /**
     * Gets the search capabilities.
     * 
     * @return the search capabilities
     */
    public ArgumentList getSearchCapabilities() {
        Logger.v(tag, "getSearchCapabilities( )");
        org.cybergarage.upnp.Service service = mDevice.getService(Upnp.Service.SERVICE_CDS1);
        if (null == service) {
            return null;
        }

        Action action = service.getAction(Upnp.CDSAction.CDS_ACTION_GET_SEARCH_CAPABILITIES);
        if (null == action) {
            return null;
        }
        if (action.postControlAction()) {
            ArgumentList outArgList = action.getOutputArgumentList();
            UpnpTool.printArgContent(Upnp.CDSAction.CDS_ACTION_GET_SEARCH_CAPABILITIES, outArgList,
                    Upnp.CDSArgVariable.GetSearchCapabilities.CDS_VARIABLE_OUT_SEARCH_CAPS);
            return outArgList;
        } else {
            UPnPStatus err = action.getControlStatus();
            Logger.e(tag, "Error Code = " + err.getCode());
            Logger.e(tag, "Error Desc = " + err.getDescription());
            return null;
        }
    }

    /**
     * Gets the sort capabilities.
     * 
     * @return the sort capabilities
     */
    public ArgumentList getSortCapabilities() {
        Logger.v(tag, "getSortCapabilities( )");
        org.cybergarage.upnp.Service service = mDevice.getService(Upnp.Service.SERVICE_CDS1);
        if (null == service) {
            return null;
        }

        Action action = service.getAction(Upnp.CDSAction.CDS_ACTION_GET_SORT_CAPABILITIES);
        if (null == action) {
            return null;
        }
        if (action.postControlAction()) {
            ArgumentList outArgList = action.getOutputArgumentList();
            UpnpTool.printArgContent(Upnp.CDSAction.CDS_ACTION_GET_SORT_CAPABILITIES, outArgList,
                    Upnp.CDSArgVariable.GetSortCapabilities.CDS_VARIABLE_OUT_SORT_CAPS);
            return outArgList;
        } else {
            UPnPStatus err = action.getControlStatus();
            Logger.e(tag, "Error Code = " + err.getCode());
            Logger.e(tag, "Error Desc = " + err.getDescription());
            return null;
        }
    }

    /**
     * Gets the sort extension capabilities.
     * 
     * @return the sort extension capabilities
     */
    public ArgumentList getSortExtensionCapabilities() {
        Logger.v(tag, "getSortExtensionCapabilities( )");
        org.cybergarage.upnp.Service service = mDevice.getService(Upnp.Service.SERVICE_CDS1);
        if (null == service) {
            return null;
        }

        Action action = service.getAction(Upnp.CDSAction.CDS_ACTION_GET_SORT_EXTENSION_CAPABILITIES);
        if (null == action) {
            return null;
        }
        if (action.postControlAction()) {
            ArgumentList outArgList = action.getOutputArgumentList();
            UpnpTool.printArgContent(Upnp.CDSAction.CDS_ACTION_GET_SORT_EXTENSION_CAPABILITIES, outArgList,
                    Upnp.CDSArgVariable.GetSortExtensionCapabilities.CDS_VARIABLE_OUT_SORT_EXTENSION_CAPS);
            return outArgList;
        } else {
            UPnPStatus err = action.getControlStatus();
            Logger.e(tag, "Error Code = " + err.getCode());
            Logger.e(tag, "Error Desc = " + err.getDescription());
            return null;
        }
    }

    /**
     * Gets the system update id.
     * 
     * @return the system update id
     */
    public ArgumentList getSystemUpdateID() {
        Logger.v(tag, "getSystemUpdateID( )");
        org.cybergarage.upnp.Service service = mDevice.getService(Upnp.Service.SERVICE_CDS1);
        if (null == service) {
            return null;
        }

        Action action = service.getAction(Upnp.CDSAction.CDS_ACTION_GET_SYSTEM_UPDATE_ID);
        if (null == action) {
            return null;
        }
        if (action.postControlAction()) {
            ArgumentList outArgList = action.getOutputArgumentList();
            UpnpTool.printArgContent(Upnp.CDSAction.CDS_ACTION_GET_SYSTEM_UPDATE_ID, outArgList,
                    Upnp.CDSArgVariable.GetSystemUpdateID.CDS_VARIABLE_OUT_ID);
            return outArgList;
        } else {
            UPnPStatus err = action.getControlStatus();
            Logger.e(tag, "Error Code = " + err.getCode());
            Logger.e(tag, "Error Desc = " + err.getDescription());
            return null;
        }
    }

    /**
     * Gets the transfer progress.
     * 
     * @param TransferID the transfer id
     * @return the transfer progress
     */
    public ArgumentList getTransferProgress(String TransferID) {
        Logger.v(tag, "getTransferProgress( )");
        org.cybergarage.upnp.Service service = mDevice.getService(Upnp.Service.SERVICE_CDS1);
        if (null == service) {
            return null;
        }

        Action action = service.getAction(Upnp.CDSAction.CDS_ACTION_GET_TRANSFER_PROGRESS);
        if (null == action) {
            return null;
        }
        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.CDSArgVariable.GetTransferProgress.CDS_VARIABLE_IN_TRANSFER_ID).setValue(
                TransferID);

        if (action.postControlAction()) {
            ArgumentList outArgList = action.getOutputArgumentList();
            UpnpTool.printArgContent(Upnp.CDSAction.CDS_ACTION_GET_TRANSFER_PROGRESS, outArgList,
                    Upnp.CDSArgVariable.GetTransferProgress.CDS_VARIABLE_OUT_TRANSFER_LENGTH);
            UpnpTool.printArgContent(Upnp.CDSAction.CDS_ACTION_GET_TRANSFER_PROGRESS, outArgList,
                    Upnp.CDSArgVariable.GetTransferProgress.CDS_VARIABLE_OUT_TRANSFER_STATUS);
            UpnpTool.printArgContent(Upnp.CDSAction.CDS_ACTION_GET_TRANSFER_PROGRESS, outArgList,
                    Upnp.CDSArgVariable.GetTransferProgress.CDS_VARIABLE_OUT_TRANSFER_TOTAL);
            return outArgList;
        } else {
            UPnPStatus err = action.getControlStatus();
            Logger.e(tag, "Error Code = " + err.getCode());
            Logger.e(tag, "Error Desc = " + err.getDescription());
            return null;
        }
    }

    /**
     * Import resource.
     * 
     * @param SourceURI the source uri
     * @param DestinationURI the destination uri
     * @return the argument list
     */
    public ArgumentList importResource(String SourceURI, String DestinationURI) {
        Logger.v(tag, "importResource(" + SourceURI + "," + DestinationURI + ")");
        org.cybergarage.upnp.Service service = mDevice.getService(Upnp.Service.SERVICE_CDS1);
        if (null == service) {
            return null;
        }

        Action action = service.getAction(Upnp.CDSAction.CDS_ACTION_IMPORT_RESOURCE);
        if (null == action) {
            return null;
        }
        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.CDSArgVariable.ImportResource.CDS_VARIABLE_IN_SOURCE_URI)
                .setValue(SourceURI);
        argumentList.getArgument(Upnp.CDSArgVariable.ImportResource.CDS_VARIABLE_IN_DESTINATION_URI).setValue(
                DestinationURI);

        if (action.postControlAction()) {
            ArgumentList outArgList = action.getOutputArgumentList();
            UpnpTool.printArgContent(Upnp.CDSAction.CDS_ACTION_IMPORT_RESOURCE, outArgList,
                    Upnp.CDSArgVariable.ImportResource.CDS_VARIABLE_OUT_TRANSFER_ID);
            return outArgList;
        } else {
            UPnPStatus err = action.getControlStatus();
            Logger.e(tag, "Error Code = " + err.getCode());
            Logger.e(tag, "Error Desc = " + err.getDescription());
            return null;
        }
    }

    /**
     * Move object.
     * 
     * @param ObjectID the object id
     * @param NewParentID the new parent id
     * @return the argument list
     */
    public ArgumentList moveObject(String ObjectID, String NewParentID) {
        Logger.v(tag, "moveObject(" + ObjectID + "," + NewParentID + ")");
        org.cybergarage.upnp.Service service = mDevice.getService(Upnp.Service.SERVICE_CDS1);
        if (null == service) {
            return null;
        }

        Action action = service.getAction(Upnp.CDSAction.CDS_ACTION_MOVE_OBJECT);
        if (null == action) {
            return null;
        }
        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.CDSArgVariable.MoveObject.CDS_VARIABLE_IN_OBJECT_ID).setValue(ObjectID);
        argumentList.getArgument(Upnp.CDSArgVariable.MoveObject.CDS_VARIABLE_IN_PARENT_ID)
                .setValue(NewParentID);

        if (action.postControlAction()) {
            ArgumentList outArgList = action.getOutputArgumentList();
            UpnpTool.printArgContent(Upnp.CDSAction.CDS_ACTION_MOVE_OBJECT, outArgList,
                    Upnp.CDSArgVariable.MoveObject.CDS_VARIABLE_OUT_NEW_OBJECT_ID);
            return outArgList;
        } else {
            UPnPStatus err = action.getControlStatus();
            Logger.e(tag, "Error Code = " + err.getCode());
            Logger.e(tag, "Error Desc = " + err.getDescription());
            return null;
        }
    }

    /**
     * Search.
     * 
     * @param ContainerID the container id
     * @param SearchCriteria the search criteria
     * @param Filter the filter
     * @param StartingIndex the starting index
     * @param RequestedCount the requested count
     * @param SortCriteria the sort criteria
     * @return true, if successful
     */
    public ArgumentList Search(String ContainerID, String SearchCriteria, String Filter,
            String StartingIndex, String RequestedCount, String SortCriteria) {
        Logger.v(tag, "Search(" + ContainerID + "," + SearchCriteria + "," + Filter + ","
                + StartingIndex + "," + RequestedCount + "," + SortCriteria + ")");
        org.cybergarage.upnp.Service service = mDevice.getService(Upnp.Service.SERVICE_CDS1);
        if (null == service) {
            return null;
        }

        Action action = service.getAction(Upnp.CDSAction.CDS_ACTION_SEARCH);
        if (null == action) {
            return null;
        }
        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.CDSArgVariable.Search.CDS_VARIABLE_IN_CONTAINER_ID).setValue(ContainerID);
        argumentList.getArgument(Upnp.CDSArgVariable.Search.CDS_VARIABLE_IN_SEARCH_CRITERIA).setValue(
                SearchCriteria);
        argumentList.getArgument(Upnp.CDSArgVariable.Search.CDS_VARIABLE_IN_STARTING_INDEX)
                .setValue(StartingIndex);
        argumentList.getArgument(Upnp.CDSArgVariable.Search.CDS_VARIABLE_IN_REQUESTED_COUNT).setValue(
                RequestedCount);
        argumentList.getArgument(Upnp.CDSArgVariable.Search.CDS_VARIABLE_IN_FILTER).setValue(Filter);
        argumentList.getArgument(Upnp.CDSArgVariable.Search.CDS_VARIABLE_IN_SORT_CRITERIA).setValue(SortCriteria);

        if (action.postControlAction()) {
            ArgumentList outArgList = action.getOutputArgumentList();
            UpnpTool.printArgContent(Upnp.CDSAction.CDS_ACTION_SEARCH, outArgList,
                    Upnp.CDSArgVariable.Search.CDS_VARIABLE_OUT_RESULT);
            UpnpTool.printArgContent(Upnp.CDSAction.CDS_ACTION_SEARCH, outArgList,
                    Upnp.CDSArgVariable.Search.CDS_VARIABLE__OUT_NUMBER_RETURNED);
            UpnpTool.printArgContent(Upnp.CDSAction.CDS_ACTION_SEARCH, outArgList,
                    Upnp.CDSArgVariable.Search.CDS_VARIABLE_OUT_TOTAL_MATCHED);
            UpnpTool.printArgContent(Upnp.CDSAction.CDS_ACTION_SEARCH, outArgList,
                    Upnp.CDSArgVariable.Search.CDS_VARIABLE_OUT_UPDATE_ID);
            return outArgList;
        } else {
            UPnPStatus err = action.getControlStatus();
            Logger.e(tag, "Error Code = " + err.getCode());
            Logger.e(tag, "Error Desc = " + err.getDescription());
            return null;
        }
    }

    /**
     * Stop transfer resource.
     * 
     * @param TransferID the transfer id
     * @return true, if successful
     */
    public boolean StopTransferResource(String TransferID) {
        Logger.v(tag, "StopTransferResource(" + TransferID + ")");
        org.cybergarage.upnp.Service service = mDevice.getService(Upnp.Service.SERVICE_CDS1);
        if (null == service) {
            return false;
        }

        Action action = service.getAction(Upnp.CDSAction.CDS_ACTION_STOP_TRANSFER_RESOURCE);
        if (null == action) {
            return false;
        }
        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.CDSArgVariable.StopTransferResource.CDS_VARIABLE_IN_TRANSFER_ID).setValue(
                TransferID);
        if (action.postControlAction()) {
            return true;
        } else {
            UPnPStatus err = action.getControlStatus();
            Logger.e(tag, "Error Code = " + err.getCode());
            Logger.e(tag, "Error Desc = " + err.getDescription());
            return false;
        }
    }

    /**
     * Update object.
     * 
     * @param ObjectID the object id
     * @param CurrentTagValue the current tag value
     * @param NewTagValue the new tag value
     * @return true, if successful
     */
    public boolean UpdateObject(String ObjectID, String CurrentTagValue, String NewTagValue) {
        Logger.v(tag, "UpdateObject(" + ObjectID + "," + CurrentTagValue + "," + NewTagValue + ")");
        org.cybergarage.upnp.Service service = mDevice.getService(Upnp.Service.SERVICE_CDS1);
        if (null == service) {
            return false;
        }

        Action action = service.getAction(Upnp.CDSAction.CDS_ACTION_UPDATE_OBJECT);
        if (null == action) {
            return false;
        }
        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.CDSArgVariable.UpdateObject.CDS_VARIABLE_IN_OBJECT_ID).setValue(ObjectID);
        argumentList.getArgument(Upnp.CDSArgVariable.UpdateObject.CDS_VARIABLE_IN_CURRENT_TAG_VALUE).setValue(
                CurrentTagValue);
        argumentList.getArgument(Upnp.CDSArgVariable.UpdateObject.CDS_VARIABLE_IN_NEW_TAG_VALUE).setValue(
                NewTagValue);

        if (action.postControlAction()) {
            return true;
        } else {
            UPnPStatus err = action.getControlStatus();
            Logger.e(tag, "Error Code = " + err.getCode());
            Logger.e(tag, "Error Desc = " + err.getDescription());
            return false;
        }
    }
}
