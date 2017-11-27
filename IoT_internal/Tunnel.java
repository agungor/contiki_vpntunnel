import java.io.IOException;
import java.net.DatagramPacket;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.DatagramSocket;

public class Tunnel{
	private final int PORT=9000;//IEP receives connections on this port
	private static final int DATA_SIZE=200;
	byte [] rcvDataBuf=new byte[DATA_SIZE]; //a buffer to put the data from 6EP in.
	String sicsEPaddrString="130.237.20.137";//6EP IP address
	InetAddress sixEpAddress;
	int remotPort;
	public StateTable stable;

	public Tunnel(){
		start(); //when we create an object of this type, start will directly run.
	}
	private void start(){
		try {
			System.out.println("will listen and accept a new connection");
			DatagramSocket connectionSocket=new DatagramSocket(PORT);
			DatagramPacket rcvPkt=new DatagramPacket(this.rcvDataBuf, rcvDataBuf.length);
			int pktLength=rcvPkt.getLength();
			this.sixEpAddress=InetAddress.getByName(this.sicsEPaddrString);	

			while (true) {
				System.out.println("waiting for a packet");
				connectionSocket.receive(rcvPkt); //by this line, our buffer rcvDataBuf is full with data.
				this.remotPort=rcvPkt.getPort();
				System.out.print("packet received from 6EP, source port: "+ this.remotPort);
				System.out.println(", packet lengh: "+ pktLength);
				
				/*
				 * The if-statement below is to be activated when we test on hardware.
				 * For now we are testing in local machine.
				 * */
				if(connectionSocket.getInetAddress()==this.sixEpAddress){
					//process_packet_from_6ep
				}
				else{
					//process_packet_to_6ep
				}
				process_packet_from_6ep(connectionSocket, rcvPkt, pktLength); //for demo purpose only
				//process_packet_to_6ep(connectionSocket, rcvPkt, pktLength);
				
			}
			} catch (IOException e) {
				e.printStackTrace();
			}
	}
	
	/*
	 * The function process_connection takes a connection that is received on a socket and pass it to the tunnel.
	 * The tunnel is implemented on a separate thread
	 * */
	private void process_packet_from_6ep(DatagramSocket socket, DatagramPacket packet, int pktLength){
		SixEpReader readFrom6Ep=new SixEpReader(this, socket, packet, pktLength);
		new Thread(readFrom6Ep).start();
		synchronized(readFrom6Ep){
    		try {
    			readFrom6Ep.wait();
    		}catch(InterruptedException e){
    			e.printStackTrace();
    		}
    	}
		SixEpWriter writeTo6Ep=new SixEpWriter(this, this.remotPort, packet, pktLength, readFrom6Ep);
		new Thread(writeTo6Ep).start();
		
	}
	
    public static void main(String[] args) {
    	Tunnel tun=new Tunnel();
    	/*StateTable st=new StateTable();
		StateTableEntry e=new StateTableEntry();
		e.setIep_sending_port(10);
		e.setNode_ipv6(sixEpAddress);
		e.setNode_src_port(11);
		e.setProtocol_type(1);
		e.setRemote_server_ipv6(sixEpAddress);
		e.setRemote_server_port(12);
		e.setTimestamp();
		st.add(e);
		st.remove(e);*/
    	
    }
}