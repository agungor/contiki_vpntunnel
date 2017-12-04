import java.io.ByteArrayOutputStream;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;

public class SixEpSimulator {

    public static void main(String[] args) throws Exception {
        DatagramSocket clientSocket = new DatagramSocket();
        InetAddress IPAddress = InetAddress.getByName("localhost");
        ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
        int srcPort = 1515;
        InetAddress srcAddr = InetAddress.getByName("2001:0DB8:AC10:FE01:0000:0000:0000:0000");
        int dstPort = 1516;
        InetAddress dstAddr = InetAddress.getByName("2001:0DB8:AC10:FE01:0000:0000:0000:0001");
        String sentence = "Hello";
        outputStream.write(srcAddr.getAddress());
        outputStream.write((byte) srcPort & 0xFF);
        outputStream.write((byte) (srcPort >> 8) & 0xFF);
        outputStream.write(dstAddr.getAddress());
        outputStream.write((byte) dstPort & 0xFF);
        outputStream.write((byte) (dstPort >> 8) & 0xFF);
        outputStream.write(sentence.getBytes());
        byte out[] = outputStream.toByteArray();
        DatagramPacket sendPacket = new DatagramPacket(out, out.length, IPAddress, 9000);
        clientSocket.send(sendPacket);
    }

