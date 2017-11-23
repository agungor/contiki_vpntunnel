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
	String sicsEPaddr="130.237.20.137";//6EP IP address
	InetAddress sepAddress;
	private int sepPort=8000;
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
			this.sepAddress=InetAddress.getByName(this.sicsEPaddr);		
		}catch(IOException e){
			System.out.println("ioexception");
		}
		this.tun=tun;
		this.socket=socket;
		this.rcvPkt=rcvPacket;
		this.rcvDataBuf=tun.rcvDataBuf;
		this.pktLength=pktLength;
	}
	@Override
	public void run(){
			//The if-statement below is to be activated when encap functionality is ready at 6ep side
			decap();
			//encap(this.rcvPkt);//to be moved after else when remote servers are involved
	}
		/*public void encap(DatagramSocket socket){
		
	}*/
	public void decap(){
		System.out.println("inside decap");
		//this.socket.receive(dp); //by this line, our buffer rcvDataBuf is full with data.
		//fill the ipv6 source array
		int j=0;
		int position=0;
		int currentPosition=position;
		System.out.println("storing bytes 0-15");
		for(int i=currentPosition; i<this.ipv6Src.length+currentPosition; i++){
			this.ipv6Src[j]=this.rcvDataBuf[i];
			System.out.print((char)this.ipv6Src[j]);
			j++;
			position=i;
		}
		System.out.println();
		currentPosition=position+1;
		
		//fill the src port array
		j=0;
		System.out.println("storing bytes 16-17");
		for(int i=currentPosition; i<this.portSrc.length+currentPosition; i++){
			this.portSrc[j]=this.rcvDataBuf[i];
			System.out.print((char)this.portSrc[j]);
			j++;
			position=i;
		}
		System.out.println();
		currentPosition=position+1;

		//fill the ipv6 dst array
		j=0;
		System.out.println("storing bytes 18-33");
		for(int i=currentPosition; i<this.ipv6Dst.length+currentPosition; i++){
			this.ipv6Dst[j]=this.rcvDataBuf[i];
			System.out.print((char)this.ipv6Dst[j]);
			j++;
			position=i;
		}
		System.out.println();
		currentPosition=position+1;
		
		//fill the dst port array
		j=0;
		System.out.println("storing bytes 33-35");
		for(int i=currentPosition; i<portDst.length+currentPosition; i++){
			this.portDst[j]=this.rcvDataBuf[i];
			System.out.print((char)this.portDst[j]);
			j++;
			position=i;
		}
		System.out.println();
		currentPosition=position+1;
		
		//fill the appData buffer
		j=0;
		System.out.println("storing the rest of the data ");
		
		this.appData=new byte[pktLength-currentPosition];
		for( int i=currentPosition; i<pktLength; i++){
			this.appData[j]=this.rcvDataBuf[i];
			System.out.print((char)this.appData[j]);
			j++;
		}
		synchronized(this){
    		this.notify();
    	}
		//translate final dst ip address 6to4
		//add entry to NAT table
		//send app data to ranslated address.
		
	}

}