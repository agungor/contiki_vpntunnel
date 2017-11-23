import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;

public class SixEpSimulator {

	public static void main(String[] args) throws Exception {
		DatagramSocket clientSocket=new DatagramSocket();
		InetAddress IPAddress = InetAddress.getByName("localhost");
		byte[] sendData = new byte[1024];
		String sentence = "this_is_a_message_from_the_user_Hopefully_it_will_work_or_otherwise_I_will_not_tell_you_what_I_will_do";
		sendData = sentence.getBytes();
		DatagramPacket sendPacket = new DatagramPacket(sendData, sendData.length, IPAddress, 5151);
		clientSocket.send(sendPacket);
	}
}