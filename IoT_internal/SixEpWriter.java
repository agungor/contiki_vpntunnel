import java.net.DatagramPacket;
import java.net.InetAddress;
import java.net.Socket;
import java.net.DatagramSocket;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.nio.Buffer;
import java.nio.ByteBuffer;
import java.io.IOException;
import java.io.UncheckedIOException;

public class SixEpWriter implements Runnable{
	Tunnel tun;
	//DatagramSocket socket;
	String sicsEPaddr="130.237.20.137";//6EP IP address
	InetAddress sepAddress;
	private int sepPort=666;
	private static final int DATA_SIZE=200;
	byte [] rcvDataBuf=new byte[DATA_SIZE]; //a buffer to put the data from 6EP in.
	ByteBuffer outDataBuf=ByteBuffer.allocateDirect(DATA_SIZE);
	/*byte[]ipv6Src=new byte [16]; //for demo pupose only
	byte[]portSrc=new byte [2]; //for demo pupose only
	byte[]ipv6Dst=new byte [16]; //for demo pupose only
	byte[]portDst=new byte [2]; //for demo pupose only*/
	byte[]appData; //for demo pupose only
	DatagramPacket rcvPkt;
	int pktLength;
	SixEpReader reader;
	int remotePort;
	public SixEpWriter(Tunnel tun, int remotePort, DatagramPacket rcvPacket, int pktLength, SixEpReader reader){
		try{
			this.sepAddress=InetAddress.getByName(this.sicsEPaddr);		
		}catch(IOException e){
			System.out.println("ioexception");
		}
		this.tun=tun;
		this.rcvPkt=rcvPacket;
		this.rcvDataBuf=tun.rcvDataBuf;
		this.pktLength=pktLength;
		this.reader=reader;
		this.remotePort=remotePort;
	}
	
	
	
	@Override
	public void run(){
		try {
			encap();
		} catch (UnknownHostException e) {
			e.printStackTrace();
		}
	}
	public void encap() throws UnknownHostException{
		System.out.println("\ninside encap");
		//the following part of code will be activated when we have communication with MQTT broker.
		/*this.socket.receive(dp); //by this line, our buffer rcvDataBuf is full with data.
		for(int i=0; i<dp.getLength(); i++){
			this.appData=new byte[dp.getLength()];
			this.appData[i]=this.rcvDataBuf[i];
		}*/
		this.appData=new byte [5];
		this.appData[0]=(byte) 0x68;//h
		this.appData[1]=(byte) 0x65;//e
		this.appData[2]=(byte) 0x6c;//l
		this.appData[3]=(byte) 0x6c;//l
		this.appData[4]=(byte) 0x6f;//o
		
		/*
		 * The following data is appended to the buffer to be sent:
		 * src ipv6 address
		 * port source
		 * dst ipv6 address
		 * port dst
		 * application data
		 * */
		this.outDataBuf.clear();
		this.outDataBuf.put(this.reader.ipv6Src);
		this.outDataBuf.put(this.reader.portSrc);
		this.outDataBuf.put(this.reader.ipv6Dst);
		this.outDataBuf.put(this.reader.portDst);
		this.outDataBuf.put(this.appData);
		
		//add the buffer to the DatagramPacket object

		//this.sepAddress=dp.getAddress(); //for demo purpose only
		System.out.print("ipStr:");
		InetAddress laddr=InetAddress.getByName("localhost");
		System.out.println(laddr.getHostAddress());
		System.out.print("remotePort:");
		//int remotePort=this.socket.getPort();
		System.out.println(this.remotePort);
		//this.sepPort=dp.getPort(); //for demo purpose only
		//DatagramPacket pktToBeSent=new DatagramPacket(this.outDataBuf.array(), this.outDataBuf.array().length, this.sepAddress, this.sepPort);

		this.outDataBuf.flip();
		byte [] outDataArray=new byte[this.outDataBuf.limit()];
		this.outDataBuf.get(outDataArray);
		for(int i=0; i<outDataArray.length; i++){
			System.out.print("tha array to be sent as a reply contains the folloing info:"+(char)outDataArray[i]+"\n");
		}
		
		DatagramPacket pktToBeSent=new DatagramPacket(outDataArray, outDataArray.length, laddr, this.remotePort);
		
		try {
			DatagramSocket socket=new DatagramSocket();
			socket.send(pktToBeSent);
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
}
