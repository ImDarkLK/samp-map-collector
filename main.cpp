
#include <windows.h>
#include <process.h>
#include <stdio.h>

#include <vector>

#include "BitStream.h"

#define SAMP_VERSION_STR 0x10117BFC
#define GAME_INITED_GLB 0x102ACA40 // BOOL bGameInited;
#define CMD_WINDOW_PTR  0x102ACA14 // CCmdWindow* pCmdWindow;
#define ADD_CMD_SUB 0x100691B0 // CCmdWindow::AddDefaultCmdProc(char*,void*);
#define CHAT_WINDOW_PTR 0x102ACA10 // CChatWindow* pChatWindow;
#define ADD_DEBUG_MSG_SUB 0x10067B60 // CChatWindow::AddDebugMessage(char*,...);
#define CREATE_OBJECT_RPC_SUB 0x1001AE70 // ScrCreateObject(RPCParams*);
#define REMOVE_BUILDING_RPC_SUB 0x1001CFF0 // ScrRemoveBuilding(RPCParams*);

// patch addresses
#define CREATE_OBJECT_DETOUR 0x1001DC80
#define REMOVE_BUILDING_DETOUR 0x1001E17B

#define SAMP_IMAGE_BASE 0x10000000

#define ADDR(x) (samp_dll + ((x) - SAMP_IMAGE_BASE))

#define PREFIX "{CB123C}samp-map-collector: {FFFFFF}"

void log(char* format, ...);

struct rpc_params_stru
{
	unsigned char* data;
	unsigned lenght;
};

struct create_object_stru
{
	unsigned id;
	unsigned short objectid;
	int modelid;
	float x,y,z;
	float rx,ry,rz;
	float drawdist;
};

struct remove_building_stru
{
	int modelid;
	float x, y, z;
	float radius;
};

DWORD samp_dll = 0;
unsigned object_count = 0;
bool do_collecting = true;

std::vector<create_object_stru*> create_objects;
std::vector<remove_building_stru*> remove_buildings;

void add_object_to_collection(rpc_params_stru* rpc_params)
{
	if (do_collecting)
	{
		RakNet::BitStream bsData(rpc_params->data, rpc_params->lenght / 8, false);
		create_object_stru* co = new create_object_stru();

		bsData.Read(co->objectid);
		bsData.Read(co->modelid);
		bsData.Read(co->x);
		bsData.Read(co->y);
		bsData.Read(co->z);
		bsData.Read(co->rx);
		bsData.Read(co->ry);
		bsData.Read(co->rz);
		bsData.Read(co->drawdist);

		bool in_the_list = false;
		for (create_object_stru* i : create_objects)
		{
			if (//i->objectid == co->objectid &&
				i->modelid == co->modelid &&
				i->x == co->x &&
				i->y == co->y &&
				i->z == co->z &&
				i->rx == co->rx &&
				i->ry == co->ry &&
				i->rz == co->rz &&
				i->drawdist == co->drawdist)
			{
				in_the_list = true;
				break;
			}
		}

		if (!in_the_list)
		{
			co->id = object_count++;
			create_objects.push_back(co);
		}
	}

	// call ScrCreateObject(RPCParams*)
	DWORD _func = ADDR(CREATE_OBJECT_RPC_SUB);
	_asm push rpc_params
	_asm call _func
}

void remove_building_rpc(rpc_params_stru* rpc_params)
{
	if (do_collecting)
	{
		RakNet::BitStream bsData(rpc_params->data, rpc_params->lenght / 8, false);
		remove_building_stru* rbs = new remove_building_stru();

		bsData.Read(rbs->modelid);
		bsData.Read(rbs->x);
		bsData.Read(rbs->y);
		bsData.Read(rbs->z);
		bsData.Read(rbs->radius);

		bool in_the_list = false;
		for (remove_building_stru* i : remove_buildings)
		{
			if (i->modelid == rbs->modelid &&
				i->x == rbs->x &&
				i->y == rbs->y &&
				i->z == rbs->z &&
				i->radius == rbs->radius)
			{
				in_the_list = true;
				break;
			}
		}

		if (!in_the_list)
			remove_buildings.push_back(rbs);
	}

	// call ScrRemoveBuilding(RPCParams*)
	DWORD _func = ADDR(REMOVE_BUILDING_RPC_SUB);
	_asm push rpc_params
	_asm call _func
}

void add_command(char* command_name, DWORD command_func)
{
	unsigned _this = ADDR(CMD_WINDOW_PTR);
	unsigned _function = ADDR(ADD_CMD_SUB);

	_asm mov eax, dword ptr [_this]
	_asm mov ecx, dword ptr [eax]
	_asm push command_func
	_asm push command_name
	_asm call _function
}

void add_chat(char* _message)
{
	unsigned _this = ADDR(CHAT_WINDOW_PTR);
	unsigned _func = ADDR(ADD_DEBUG_MSG_SUB);

	_asm mov ecx, dword ptr [_this]
	_asm mov eax, dword ptr [ecx]
	_asm push _message
	_asm push eax
	_asm call _func
	_asm add esp, 8
}

void _cmd_start(char* p)
{
	if (!do_collecting) {
		add_chat(PREFIX "started");
		do_collecting = true;
	}
	else
		add_chat(PREFIX "already collecting incoming data");
}

void _cmd_stop(char* p)
{
	if(do_collecting) {
		add_chat(PREFIX "stopped");
		do_collecting = false;
	}
	else
		add_chat(PREFIX "currently not collecting data");
}

void _cmd_clear(char* p)
{
	for (create_object_stru* i : create_objects)
	{
		delete i;
	}
	create_objects.clear();

	for (remove_building_stru* i : remove_buildings)
	{
		delete i;
	}
	remove_buildings.clear();

	add_chat(PREFIX "cleared");
}

void _cmd_stats(char* p)
{
	add_chat(PREFIX "current stats:");

	char buf[100];
	sprintf(buf, "    CreateObject count = %d", create_objects.size());
	add_chat(buf);

	sprintf(buf, "    RemoveBuildingForPlayer count = %d", remove_buildings.size());
	add_chat(buf);
}

void _cmd_dump(char* p)
{
	char filename[MAX_PATH];
	if (p != NULL && p[0] != 0) {
		sprintf(filename, "%s.pwn", p);
	} else {
		static int file_count = 0;
		sprintf(filename, "samp-map-collection-%d.pwn", file_count++);
	}

	FILE *f = fopen(filename, "a");
	if (!f) {
		add_chat(PREFIX "file creation failed");
		return;
	}

	fputs("#include <a_samp>\n\n"
		"public OnGameModeInit()\n{\n", f);

	for (create_object_stru* i : create_objects)
	{
		fprintf(f, "\tCreateObject(%d, %f, %f, %f, %f, %f, %f, %f); // %d\n",
			i->modelid,
			i->x, i->y, i->z,
			i->rx, i->ry, i->rz,
			i->drawdist,
			i->id);
	}
	fputs("\n\treturn 0\n}\n\n",f);

	fputs("public OnPlayerConnect(playerid)\n{\n", f);
	for (remove_building_stru* i : remove_buildings)
	{
		fprintf(f, "\tRemoveBuildingForPlayer(playerid, %d, %f, %f, %f, %f);\n",
			i->modelid,
			i->x, i->y, i->z,
			i->radius);
	}
	fputs("\n\treturn 1;\n}\n\n",f);

	char buf[MAX_PATH];
	sprintf(buf, PREFIX "dump finished to %s", filename);
	add_chat(buf);

	fclose(f);
}

void log(char* format, ...)
{
	FILE* f = fopen("samp-map-collector.log", "a");
	if (!f) return;

	va_list vl;
	va_start(vl, format);
	vfprintf(f, format, vl);
	va_end(vl);

	fflush(f);
	fclose(f);
}

void wait_thread(PVOID v)
{
	// wait until the mod initializes
	while (*(BOOL*)ADDR(GAME_INITED_GLB) != TRUE)
	{
		Sleep(50);
	}

	add_command("start_record", (DWORD)&_cmd_start);
	add_command("stop_record", (DWORD)&_cmd_stop);
	add_command("clear_record", (DWORD)&_cmd_clear);
	add_command("record_stats", (DWORD)&_cmd_stats);
	add_command("dump_record", (DWORD)&_cmd_dump);

	*(DWORD*)(ADDR(CREATE_OBJECT_DETOUR)) = (DWORD)&add_object_to_collection;
	*(DWORD*)(ADDR(REMOVE_BUILDING_DETOUR)) = (DWORD)&remove_building_rpc;

	add_chat(PREFIX "started (/start_record)");
	add_chat(PREFIX "to stop the record, use /stop_record");
	add_chat(PREFIX "if you finish collecting objects, then use /dump_record");
	add_chat(PREFIX "you can also use /clear_record to delete currently recorded objects");

	// keep alive
	while (TRUE)
	{
		Sleep(50);
	}

	ExitThread(0);
}

void init()
{
	samp_dll = (DWORD)GetModuleHandleA("samp.dll");
	if (samp_dll == NULL)
	{
		return;
	}
	
	DWORD old_prot;
	if (!VirtualProtect((PVOID)samp_dll, 0x2AD6BC, PAGE_EXECUTE_READWRITE, &old_prot))
		log("VirtualProtect failed\n");

	if (memcmp((PCHAR)ADDR(SAMP_VERSION_STR), "0.3.DL-R1", 10) != 0)
	{
		log("unsupportedd sa-mp version\n");
		return;
	}

	_beginthread(wait_thread, 0, NULL);
}

BOOL WINAPI DllMain(
	HINSTANCE hinstDLL,
	DWORD fdwReason,
	LPVOID lpReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hinstDLL);

		init();
	}
	else if (fdwReason == DLL_PROCESS_DETACH)
	{
		log("unloaded\n");
	}
	return TRUE;
}
