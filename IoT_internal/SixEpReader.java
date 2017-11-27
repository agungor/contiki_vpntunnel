import java.net.DatagramPacket;
import java.net.InetAddress;
import java.net.Socket;
import java.net.DatagramSocket;
import java.nio.Buffer;
import java.nio.ByteBuffer;
import java.io.IOException;
import java.io.UncheckedIOException;

public class SixEpReader implements Runnable{
	Tunnel tun;
	DatagramSocket socket;
	String sixEpAddrString;
	InetAddress sixEpAddress;
	private int sixEpPort=8000;
	private static final int DATA_SIZE=200;
	byte [] rcvDataBuf=new byte[DATA_SIZE]; //a buffer to put the data from 6EP in.
	ByteBuffer outDataBuf=ByteBuffer.allocateDirect(DATA_SIZE);	
	byte[]ipv6Src=new byte [16]; //for demo pupose only
	byte[]portSrc=new byte [2]; //for demo pupose only
	byte[]ipv6Dst=new byte [16]; //for demo pupose only
	byte[]portDst=new byte [2]; //for demo pupose only
	byte[]appData; //for demo pupose only
	DatagramPacket rcvPkt;
	int pktLength;
	public SixEpReader(Tunnel tun, DatagramSocket socket, DatagramPacket rcvPacket, int pktLength){
		try{
			this.sixEpAddress=InetAddress.getByName(this.sixEpAddrString);//this will be activated if we are testing with hardware
			//this.sixEpAddress=InetAddress.getByName("localhost");//this will be activated if we are tesing in local machine
		}catch(IOException e){
			System.out.println("ioexception");
		}
		this.tun=tun;
		this.sixEpAddrString=tun.sicsEPaddrString;
		this.socket=socket;
		this.rcvPkt=rcvPacket;
		this.rcvDataBuf=tun.rcvDataBuf;
		this.pktLength=pktLength;
	}
	@Override
	public void run(){
			decap();
	}
	
	public void decap(){
		System.out.println("Decapsulation process starts...");
		int j=0;
		int position=0;
		int currentPosition=position;
		System.out.print("IPv6 address of source node: ");
		System.out.print("0x");
		/*
		 * The foor loop sequences below fill the arrays with important info to be stored.
		 * */
		
		for(int i=currentPosition; i<this.ipv6Src.length+currentPosition; i++){
			this.ipv6Src[j]=this.rcvDataBuf[i];
			System.out.printf("%02X", this.ipv6Src[j]);
			if(j%2!=0&&j>0&&j<15){
				System.out.print(":");
			}
			j++;
			position=i;
		}
		System.out.println();
		currentPosition=position+1;
		
		//fill the src port array
		j=0;
		System.out.print("Port number of the source node: ");
		System.out.print("0x");
		for(int i=currentPosition; i<this.portSrc.length+currentPosition; i++){
			this.portSrc[j]=this.rcvDataBuf[i];
			System.out.printf("%02X", this.portSrc[j]);
			j++;
			position=i;
		}
		System.out.println();
		currentPosition=position+1;

		//fill the ipv6 dst array
		j=0;
		System.out.print("IPv6 address of the remote server: ");
		System.out.print("0x");
		for(int i=currentPosition; i<this.ipv6Dst.length+currentPosition; i++){
			this.ipv6Dst[j]=this.rcvDataBuf[i];
			System.out.printf("%02X", this.ipv6Dst[j]);
			if(j%2!=0&&j>0&&j<15){
				System.out.print(":");

			}
			j++;
			position=i;
		}
		System.out.println();
		currentPosition=position+1;
		
		//fill the dst port array
		j=0;
		System.out.print("Port number at the remote server: ");
		System.out.print("0x");
		for(int i=currentPosition; i<portDst.length+currentPosition; i++){
			this.portDst[j]=this.rcvDataBuf[i];
			System.out.printf("%02X", this.portDst[j]);
			j++;
			position=i;
		}
		System.out.println();
		currentPosition=position+1;
		
		//fill the appData buffer
		j=0;
		System.out.print("Application data: ");
		this.appData=new byte[pktLength-currentPosition];
		for( int i=currentPosition; i<pktLength; i++){
			this.appData[j]=this.rcvDataBuf[i];
			System.out.print((char)this.appData[j]);
			j++;
		}
		synchronized(this){
    		this.notify();
    	}
		System.out.println();
		/*
		 * To-do in next phase:
		 * translate final dst ip address 6to4
		 * add entry to state table
		 * send app data to translated address.
		 * */
	}

}