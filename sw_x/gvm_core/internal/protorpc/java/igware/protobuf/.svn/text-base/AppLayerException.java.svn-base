package igware.protobuf;

public class AppLayerException extends ProtoRpcException {

    private static final long serialVersionUID = -3264267950883345984L;
    private final int appStatus;

    public AppLayerException(int appStatus)
    {
        super("AppStatus = " + appStatus);
        this.appStatus = appStatus;
    }

    public int getAppStatus()
    {
        return appStatus;
    }
}
