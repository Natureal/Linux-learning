#include <unistd.h>
#include <stdio.h>

// another story: swith to user from root
static bool switch_to_user(uid_t user_id, gid_t gp_id){
	// ensure object user is not root
	if((user_id == 0) && (gp_id == 0)){
		return false;
	}
	// ensure current user is a legal one: root / object user
	gid_t gid = getgid();
	uid_t uid = getuid();
	if(((gid != 0) || (uid != 0)) && ((gid != gp_id) || (uid != user_id))){
		return false;
	}
	// if not root, then object user
	if(uid != 0){
		return true;
	}
	// swith to object user
	if((setgid(gp_id) < 0) || (setuid(user_id) < 0)){
		return false;
	}
	return true;
}


int main(){
	uid_t uid = getuid();
	uid_t euid = geteuid();
	printf("userid is %d, effective userid is: %d\n", uid, euid);
	return 0;
}

// Commands after
// g++ uid_euid.cpp -i uid_euid
// sudo chown root:root uid_euid
// sudo chmod +s uid_euid  // set-user-id flag
// ./uid_euid
