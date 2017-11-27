import java.util.ArrayList;
import java.util.List;
/**
 * This class has a list of type StateTableEntry. It has the following methods:
 * add(StateTableEntry)
 * remove(StateTableEntry)
 * get(stateTableEntry)
 * update(StateTableEntry)
 *
 */

public class StateTable {
	List<StateTableEntry> stable=new ArrayList<StateTableEntry>();
	/**
	 * This function adds an entry to the state table. But first it checks the primary keys of the table elements to avoid duplication in the table.
	 * Returns: True if the entry is added successfully.
	 *
	 */
	public boolean add(StateTableEntry entry){
		if(!contains(entry)){
			return this.stable.add(entry);
			}
		else{
			return false;
		}
	}
	
	/**
	 * This function removes the given entry from the state table and returns true if the list was found and removed.

	 */
	public boolean remove(StateTableEntry entry){
		entry=this.get(entry);
		if(entry!=null){
			return this.stable.remove(entry);
		}
		else return false;
	}
	
	public boolean contains(StateTableEntry entry){
		for(int i=0; i<this.stable.size(); i++){
			if(entry.node_ipv6==this.stable.get(i).node_ipv6&&
				entry.node_src_port==this.stable.get(i).node_src_port&&
				entry.remote_server_ipv6==this.stable.get(i).remote_server_ipv6&&
				entry.remote_server_port==this.stable.get(i).remote_server_port){
				return true;
			}
		}
		return false;//if the entry is not found		
	}
	/**
	 * This function returns null if entry not found in the table, if found it returns the entry
	 *
	 */
	public StateTableEntry get(StateTableEntry entry){
		for(int i=0; i<this.stable.size(); i++){
			if(entry.node_ipv6==this.stable.get(i).node_ipv6&&
				entry.node_src_port==this.stable.get(i).node_src_port&&
				entry.remote_server_ipv6==this.stable.get(i).remote_server_ipv6&&
				entry.remote_server_port==this.stable.get(i).remote_server_port){
				return entry;
			}
		}
		return null;//if the entry is not found		
	}
	
	/**
	 * Calling this function is equivalent to running the line entry.setTimestamp(). This function has to be called each time 
	 *
	 */
	public void updateTimeStamp(StateTableEntry entry){
		entry.setTimestamp();
	}
	
	
	
}
