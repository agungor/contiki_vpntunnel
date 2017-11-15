import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;

public class Tunnel {
	private final int PORT=5151;//IEP receives connections on this port
	
	public Tunnel(){
		start(); //when we create an object of this type, start will directly run.
	}
	private void start(){
		try {
			ServerSocket listeningSocket = new ServerSocket(PORT);
			while (true) {
				System.out.println("will listen and accept a new client");
				Socket connectionSocket = listeningSocket.accept();
				process_connection(connectionSocket);
			}
			} catch (IOException e) {
				e.printStackTrace();
			}
	}
	
	/*
	 * The function process_connection takes a connection that is received on a socket and pass it to the tunnel.
	 * The tunnel is implemented on a separate thread
	 * */
	private void process_connection(Socket socket){
		ConnectionHandler connHan=new ConnectionHandler(this, socket);
		new Thread(connHan).start();
	}
}