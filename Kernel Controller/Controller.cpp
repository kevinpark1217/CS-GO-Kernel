#include <iostream>
#include "Controller.h"
#include "Memory.h"
#include "BSP.h"

Memory game("\\\\.\\barbell");

BSP bsp;
//Input input;
RECT Rect = { 0, 0, 1920, 1080 };
HWND hwnd;

bool isPanic = false;

bool isRcs[4] = { true, true, false, false };
int aimBone = 8;
//float aimSens = 4.54545f, aimSensScope = 10.2273f;
float aimSens = 4.54545f *.8f, aimSensScope = 10.2273f*.8f;
//float aimSlow = .01f, aimFast[4] = { .26f, .23f, .31f, .28f }, aimDelta[4] = { .012f, .009f, .016f, .013f }, aimWidth = 6.0f, aimHeight = 4.f;
float aimSlow = .01f, aimFast[4] = { .28f, .25f, .31f, .29f }, aimDelta[4] = { .013f, .011f, .016f, .014f }, aimWidth = 8.0f, aimHeight = 6.f;
//float aimSlow = 10.f, aimFast[4] = { 1.f, 1.f, 1.f, 1.f }, aimDelta[4] = { 1.f, 1.f, 1.f, 1.f }, aimWidth = 180.0f, aimHeight = 90.f;

//0-pistol 1-rifle 2-awp 3-tap
bool isRunning = false, isDead = false;
int weaponMode = -1;

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	MSLLHOOKSTRUCT &p = *(PMSLLHOOKSTRUCT)lParam;
	if (p.flags & LLMHF_INJECTED || p.flags & LLMHF_LOWER_IL_INJECTED)
	{
		return 1;
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void MouseMove(float x, float y)
{
	INPUT Input = { 0 };
	Input.type = INPUT_MOUSE;
	Input.mi.dx = (LONG)(x * 10);
	Input.mi.dy = (LONG)(y * 10);
	Input.mi.dwFlags = MOUSEEVENTF_MOVE;
	SendInput(1, &Input, sizeof(INPUT));
}

void Shoot()
{
	SendMessage(hwnd, WM_SETCURSOR, (WPARAM)hwnd, 0x02010001);
	PostMessage(hwnd, WM_LBUTTONDOWN, 0x1, 0x021C03C0);
	Sleep(rand() % 10 + 175);
	SendMessage(hwnd, WM_SETCURSOR, (WPARAM)hwnd, 0x02020002);
	PostMessage(hwnd, WM_LBUTTONUP, 0x0, 0x021C03C0);
}

void Jump()
{
	PostMessage(hwnd, WM_SYSKEYDOWN, VK_MENU, 0x20380001);
	Sleep(rand() % 10 + 300);
	PostMessage(hwnd, WM_SYSKEYUP, VK_MENU, 0xC0380001);
}

bool WorldToScreen(float * from, float * to)
{
	float flMatrix[4][4] = {
		{ 0b00000000000000000000000000000000, 0b00000000000000000000000000000000, 0b00010110000010001000000010111111, 0b00100110000011101110000011000000 },
		{ 0b00000000000000000000000000000000, 0b00000000000000000000000000000000, 0b00000000000000001000000010111111, 0b00000000000000000000000000000000 },
		{ 0b10101011101010101010101000111111, 0b00000000000000000000000000000000, 0b00000000000000000000000000000000, 0b00000000000000000000000000000000 },
		{ 0b00000000000000000000000000000000, 0b00000000000000000100000000111111, 0b00000000000000000000000000000000, 0b00000000000000000000000000000000 }
	};

	float w = flMatrix[3][0] * from[0] + flMatrix[3][1] * from[1] + flMatrix[3][2] * from[2] + flMatrix[3][3];
	to[0] = flMatrix[0][0] * from[0] + flMatrix[0][1] * from[1] + flMatrix[0][2] * from[2] + flMatrix[0][3];
	to[1] = flMatrix[1][0] * from[0] + flMatrix[1][1] * from[1] + flMatrix[1][2] * from[2] + flMatrix[1][3];

	if (w < 0.01f)
		return false;

	int width = (int)(Rect.right - Rect.left);
	int height = (int)(Rect.bottom - Rect.top);

	to[0] *= (1.0f / w);
	to[1] *= (1.0f / w);

	float x = width / 2.0f;
	float y = height / 2.0f;

	x += 0.5f * to[0] * width + 0.5f;
	y -= 0.5f * to[1] * height + 0.5f;

	to[0] = x + Rect.left;
	to[1] = y + Rect.top;

	return true;

}

void worldToAngle(float* myPos, float* enemyPos, float* ang) {
	float pi = 3.14159265f;
	float xD = enemyPos[0] - myPos[0];
	float yD = enemyPos[1] - myPos[1];
	if (xD < 0 && yD < 0)
		ang[1] = (atan(yD / xD) * (180 / pi));
	else if (xD >= 0 && yD >= 0)
		ang[1] = -180 + (atan(yD / xD) * (180 / pi));
	else if (xD <= 0 && yD >= 0)
		ang[1] = (atan(yD / xD) * (180 / pi));
	else if (xD>0 && yD<0)
		ang[1] = 180 + (atan(yD / xD) * (180 / pi));
	if (ang[1] >= 180.00f)
		ang[1] = 179.99f;
	else if (ang[1] <= -180.0f)
		ang[1] = -179.99f;
	float xyD = sqrt(xD*xD + yD*yD);
	ang[0] = atan((enemyPos[2] - myPos[2]) / xyD) * (180 / pi);
	ang[1] = (float)ang[1];
	ang[0] = (float)ang[0];
}

void loadMap() {
	char newMap[128];

	game.GetMapName(newMap);
	//T:
	char pathMap[MAX_PATH] = { 31, 113, 100, 27, 57, 36, 44, 57, 42, 38, 56, 100, 24, 63, 46, 42, 38, 100, 56, 63, 46, 42, 38, 42, 59, 59, 56, 100, 40, 36, 38, 38, 36, 37, 100, 8, 36, 62, 37, 63, 46, 57, 102, 24, 63, 57, 34, 32, 46, 107, 12, 39, 36, 41, 42, 39, 107, 4, 45, 45, 46, 37, 56, 34, 61, 46, 100, 40, 56, 44, 36, 100, 38, 42, 59, 56, 100 };
	Xor(pathMap, 77);
	//C:
	//char pathMap[MAX_PATH] = { 8,113,23,27,57,36,44,57,42,38,107,13,34,39,46,56,107,99,51,115,125,98,23,24,63,46,42,38,23,56,63,46,42,38,42,59,59,56,23,40,36,38,38,36,37,23,8,36,62,37,63,46,57,102,24,63,57,34,32,46,107,12,39,36,41,42,39,107,4,45,45,46,37,56,34,61,46,23,40,56,44,36,23,38,42,59,56,23 };
	//Xor(pathMap, 88);
	strcat_s(pathMap, newMap);
	char extension[5] = { 101, 41, 56, 59, 0 };
	Xor(extension, 4);
	strcat_s(pathMap, extension);

	bsp.LoadBSP(pathMap);
	Xor(newMap, 128);
	Xor(pathMap, MAX_PATH);
}

int weaponType(int weaponId) {

	switch (weaponId)
	{
	case 1: //one tap
	case 25: //shotguns
	case 27:
	case 29:
	case 35:
	case 40: //scout
		return 3;
	case 2: //pistols
	case 3:
	case 4:
	case 30:
	case 32:
	case 36:
	case 61:
	case 63:
		return 0;
	case 7: //rifles
	case 8:
	case 10:
	case 13:
	case 14:
	case 16:
	case 17:
	case 19:
	case 24:
	case 26:
	case 28:
	case 33:
	case 34:
	case 39:
	case 60:
	case 64:
	case 11: //auto
	case 38:
		return 1;
	case 9: //awp
		return 2;
	}
	return -1;
}

void mouseInject();
void aimbot();
void bhop();

//int main()
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	char name[33] = { 8,36,62,37,63,46,57,102,24,63,57,34,32,46,113,107,12,39,36,41,42,39,107,4,45,45,46,37,56,34,61,46,0 };
	Xor(name, 32);
	hwnd = FindWindow(NULL, name);
	Xor(name, 32);

	std::thread V0(mouseInject);
	std::thread V1(aimbot);
	std::thread V2(bhop);

	while (!isPanic) {
		int Status = game.GetStatus(); //6: In Game 3: Loading
		if (Status == 6 && !isRunning) {
			Sleep(5000);
			loadMap();

			isRunning = true;
		}
		else if (Status != 6 && isRunning) {
			isRunning = false;
			isDead = true;
			weaponMode = -1;
		}
		if (isRunning)
		{
			isDead = game.IsDead(game.MaxPlayer);
			weaponMode = weaponType(game.GetWeaponId());
		}
		if (GetAsyncKeyState(VK_HOME) & 0x8000)
			isPanic = true;
	}

	V0.join();
	V1.join();
	V2.join();

	return 0;
}

void mouseInject() {
	while (!isPanic)
	{
		HHOOK g_MouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, NULL, 0);
		MSG msg;
		while (isRunning && !isDead && !isPanic)
		{
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			Sleep(1);
		}
		UnhookWindowsHookEx(g_MouseHook);
		Sleep(250);
	}
}

void bhop()
{
	while (!isPanic)
	{
		if (!isRunning || isDead)
		{
			Sleep(250);
			continue;
		}
		int flags = game.GetFlags();
		if ((GetAsyncKeyState(VK_SPACE) & 0x8000) && flags & 0x1 == 1) {
			Jump();
		}
		Sleep(5);
	}
}

void aimMeasure(float *velocity)
{
	float firstAng[3], secondAng[3];
	game.GetAngles(firstAng);
	Sleep(5);
	game.GetAngles(secondAng);
	velocity[0] = secondAng[0] - firstAng[0];
	velocity[1] = secondAng[1] - firstAng[1];

	//Too Fast
	velocity[0] /= 2;
	velocity[1] /= 2;

	if (velocity[1] > 180.f)
		velocity[1] -= 360.f;
	else if (velocity[1] < -180.f)
		velocity[1] += 360.f;

	if (velocity[0] < aimSlow)
		velocity[0] = aimSlow;
	if (velocity[1] < aimSlow)
		velocity[1] = aimSlow;

	velocity[0] = fabs(velocity[0]);
	velocity[1] = fabs(velocity[1]);
}

int aimEntity(float rangeX, float rangeY)
{
	int entity = -1;
	float boneScreen[2], boneWorld[3], myWorld[3], currentAng[3], viewOrigin[3], distance = 56755.8408624f;

	for (int i = 0; i < game.MaxPlayer; i++) {
		if (game.GetEntity(i) == NULL || game.GetEntity(i) == game.GetLocalPlayer() || game.IsDormant(i) || game.IsDead(i) || game.GetTeam(game.MaxPlayer) == game.GetTeam(i))
			continue;
		if (weaponMode == 2)
			game.GetBonePosition(boneWorld, 6, i);
		else
			game.GetBonePosition(boneWorld, 8, i);
		game.GetPosition(myWorld, game.MaxPlayer);
		game.GetViewOrigin(viewOrigin);
		myWorld[0] += viewOrigin[0];
		myWorld[1] += viewOrigin[1];
		myWorld[2] += viewOrigin[2];
		worldToAngle(boneWorld, myWorld, boneScreen);
		if (isRcs[weaponMode]) {
			float MyPunch[3];
			game.GetPunch(MyPunch);
			boneScreen[0] -= MyPunch[0] * 2.0f;
			boneScreen[1] -= MyPunch[1] * 2.0f;
		}

		game.GetAngles(currentAng);
		float tempX = fabs(boneScreen[1] - currentAng[1]);
		float tempY = fabs(boneScreen[0] - currentAng[0]);
		if (tempX > 180.f)
			tempX = 360.f - tempX;
		//std::cout << i << "; " << tempX << " " << tempY << std::endl;
		float distant = sqrtf(((myWorld[0] - boneWorld[0]) * (myWorld[0] - boneWorld[0])) + ((myWorld[1] - boneWorld[1]) * (myWorld[1] - boneWorld[1])) + ((myWorld[2] - boneWorld[2]) * (myWorld[2] - boneWorld[2])));
		float angPercent = (aimWidth - tempX) / aimWidth;
		float distantPercent = distant / distance;
		if (((tempY < rangeY && tempX < rangeX) || (distantPercent < angPercent && tempX < aimWidth && distant < distance)) && bsp.Visible(myWorld, boneWorld)) {
			rangeY = tempY;
			rangeX = tempX;
			distance = distant;
			entity = i;
		}
	}

	return entity;
}

void aimbot() {
	while (!isPanic)
	{
		if (!isRunning || isDead || weaponMode == -1)
		{
			Sleep(250);
			continue;
		}

		int entityIndex = aimEntity(aimWidth, aimHeight);

		float aimVelocity[2];
		aimMeasure(aimVelocity);

		float randomPos[3], myPos[3], enPos[3], myView[3], myAng[3], enAng[2];
		randomPos[0] = (float)rand() / RAND_MAX * (8.f) - 4.f;
		randomPos[1] = (float)rand() / RAND_MAX * (8.f) - 4.f;
		randomPos[2] = (float)rand() / RAND_MAX * (8.f) - 5.5f;
		/*randomPos[0] = (float)rand() / RAND_MAX * (8.f) - 4.f;
		randomPos[1] = (float)rand() / RAND_MAX * (8.f) - 4.f;
		randomPos[2] = (float)rand() / RAND_MAX * (8.f) - 4.f;*/
		float verticalRecoil, horizontalRecoil;
		if (weaponMode == 0)
		{
			verticalRecoil = (float)rand() / RAND_MAX * -.25f + 1.85f;
			horizontalRecoil = (float)rand() / RAND_MAX * .2f + 1.9f;
		}
		else
		{
			verticalRecoil = (float)rand() / RAND_MAX * .1f + 2.0f;
			horizontalRecoil = (float)rand() / RAND_MAX * .2f + 1.9f;
		}

		while (entityIndex != -1 && !game.GetWeaponInReload() && game.GetWeaponClip() > 0 && !game.IsDead(game.MaxPlayer) && !game.IsDead(entityIndex) && !game.IsDormant(entityIndex) && (GetAsyncKeyState(VK_XBUTTON1) & 0x8000 || (GetAsyncKeyState(VK_LBUTTON) & 0x8000 && game.GetShotsFired() > 0 && weaponMode < 2)))
		{
			int enHealth = game.GetHealth(entityIndex);
			if (weaponMode == 2 || enHealth <= 25)
				aimBone = 6;
			else if (enHealth < 50)
				aimBone = 7;
			else
				aimBone = 8;

			//Data Calculations
			game.GetBonePosition(enPos, aimBone, entityIndex);
			game.GetPosition(myPos, game.MaxPlayer);
			game.GetViewOrigin(myView);
			myPos[0] += myView[0];
			myPos[1] += myView[1];
			myPos[2] += myView[2];
			if (!bsp.Visible(myPos, enPos))
				break;
			enPos[0] += randomPos[0];
			enPos[1] += randomPos[1];
			enPos[2] += randomPos[2];
			game.GetAngles(myAng);
			worldToAngle(enPos, myPos, enAng);

			if (isRcs[weaponMode]) {
				float MyPunch[3];
				game.GetPunch(MyPunch);
				enAng[0] -= MyPunch[0] * verticalRecoil;
				enAng[1] -= MyPunch[1] * horizontalRecoil;
			}

			float difAng[2] = { enAng[0] - myAng[0], enAng[1] - myAng[1] };
			if (difAng[1] > 180.f)
				difAng[1] -= 360.f;
			else if (difAng[1] < -180.f)
				difAng[1] += 360.f;
			if (fabs(difAng[0]) > aimHeight || fabs(difAng[1]) > aimWidth)
				break;

			int aimCounter;
			int aim[2] = { (int)(fabs(difAng[0]) / aimVelocity[0]) + 1, (int)(fabs(difAng[1]) / aimVelocity[1]) + 1 };
			if (aim[0] > aim[1])
				aimCounter = aim[0];
			else
				aimCounter = aim[1];

			difAng[0] /= aimCounter;
			difAng[1] /= aimCounter;

			float sensX, sensY;
			bool isScoped = game.IsScoped();
			if (isScoped) {
				sensX = difAng[1] * aimSensScope;
				sensY = difAng[0] * aimSensScope;
			}
			else {
				sensX = difAng[1] * aimSens;
				sensY = difAng[0] * aimSens;
			}
			MouseMove(-sensX, sensY);

			if (aimCounter == 1)
			{
				//std::cout << aimCounter << std::endl;
				if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) == false)
					Shoot();
				if (weaponMode >= 2)
					aimCounter--;
			}

			if (aimEntity(aimWidth, aimHeight) != entityIndex)
				break;

			if (aimVelocity[0] < aimDelta[weaponMode] * aimCounter + aimFast[weaponMode])
				aimVelocity[0] += aimDelta[weaponMode];
			else if (aimVelocity[0] > aimDelta[weaponMode] * aimCounter + aimFast[weaponMode])
				aimVelocity[0] -= aimDelta[weaponMode];
			if (aimVelocity[1] < aimDelta[weaponMode] * aimCounter + aimFast[weaponMode])
				aimVelocity[1] += aimDelta[weaponMode];
			else if (aimVelocity[1] > aimDelta[weaponMode] * aimCounter + aimFast[weaponMode])
				aimVelocity[1] -= aimDelta[weaponMode];

			Sleep(5);
		}

		//Sleep(1);
	}
}