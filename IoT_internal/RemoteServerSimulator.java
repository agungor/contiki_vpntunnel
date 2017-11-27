import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;

/** This program simulates a remote Server sending a "hello" in a udp packet to the IEP.
 * The packet is received and processed at Tunnel.java
 *
 */
public class RemoteServerSimulator {
	public static void main(String[] args) throws Exception {
		DatagramSocket clientSocket=new DatagramSocket();
		InetAddress IPAddress = InetAddress.getByName("localhost");

		int i=0;
		for(;;){
			String str="hello:"+i;
			DatagramPacket sendPacket = new DatagramPacket(str.getBytes(), str.length(), IPAddress, 9000);
			clientSocket.send(sendPacket);
			i++;
			Thread.sleep(4000);
		}

	}

}
