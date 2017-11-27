import java.net.InetAddress;
import java.sql.Timestamp;

public class StateTableEntry {
	public InetAddress node_ipv6;//have to make sure whether this type can hold ipv6 addresses.
	public InetAddress remote_server_ipv6;
	public int node_src_port;
	public int remote_server_port;
	public Timestamp timestamp;//find the right type
	public int iep_sending_port;
	public int protocol_type;
	
	
	public Timestamp getTimestamp() {
		return timestamp;
	}
	public void setTimestamp() {
		this.timestamp = new Timestamp(System.currentTimeMillis());;
	}
	public InetAddress getNode_ipv6() {
		return node_ipv6;
	}
	public void setNode_ipv6(InetAddress node_ipv6) {
		this.node_ipv6 = node_ipv6;
	}
	public InetAddress getRemote_server_ipv6() {
		return remote_server_ipv6;
	}
	public void setRemote_server_ipv6(InetAddress remote_server_ipv6) {
		this.remote_server_ipv6 = remote_server_ipv6;
	}
	public int getNode_src_port() {
		return node_src_port;
	}
	public void setNode_src_port(int node_src_port) {
		this.node_src_port = node_src_port;
	}
	public int getRemote_server_port() {
		return remote_server_port;
	}
	public void setRemote_server_port(int remote_server_port) {
		this.remote_server_port = remote_server_port;
	}
	public int getIep_sending_port() {
		return iep_sending_port;
	}
	public void setIep_sending_port(int iep_sending_port) {
		this.iep_sending_port = iep_sending_port;
	}
	public int getProtocol_type() {
		return protocol_type;
	}
	public void setProtocol_type(int protocol_type) {
		this.protocol_type = protocol_type;
	}
	
	
}

	
	