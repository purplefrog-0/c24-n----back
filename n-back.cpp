#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <thread>
#include <iomanip>
#include <algorithm>
#include <limits>
#include <fstream>
#include <map>
#include <sstream>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <conio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

using namespace std;

// 清除控制台屏幕的函数
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// 非阻塞键盘输入检测
bool checkKeyPress() {
#ifdef _WIN32
    return _kbhit() != 0;
#else
    struct termios oldt, newt;
    int ch;
    int oldf;
    
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    
    ch = getchar();
    
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    
    if(ch != EOF) {
        ungetc(ch, stdin);
        return true;
    }
    
    return false;
#endif
}

// 获取非阻塞输入
char getKeyInput() {
    if(checkKeyPress()) {
#ifdef _WIN32
        return _getch();
#else
        return getchar();
#endif
    }
    return 0;
}

// 刺激结构体
struct Stimulus {
    int visualPosition;  // 0-8 表示3x3网格中的位置
    char auditoryLetter; // 播放的字母
};

// 成就枚举
enum Achievement {
    ACH_NOVICE,        // 初学者：完成一次测试
    ACH_MEMORY_MASTER, // 记忆大师：准确率超过90%
    ACH_FAST_THINKER,  // 快速思考者：平均响应时间<1000ms
    ACH_PERFECT_SCORE, // 完美分数：100%准确率
    ACH_CHALLENGER,    // 挑战者：完成N=5的测试
    ACH_MULTIPLAYER,   // 多人游戏：完成多人游戏
    ACH_CONSISTENT,    // 稳定发挥：连续3次准确率>80%
    ACH_TOTAL_TESTS,   // 测试达人：完成10次测试
    ACH_DUAL_EXPERT,   // 双项专家：视觉和听觉都>90%
    ACH_NINJA,         // 记忆忍者：N=3且准确率>85%
    ACH_COUNT          // 成就总数
};

// 成就名称和描述
const string ACHIEVEMENT_NAMES[ACH_COUNT] = {
    "初学者",
    "记忆大师",
    "快速思考者",
    "完美分数",
    "挑战者",
    "多人游戏",
    "稳定发挥",
    "测试达人",
    "双项专家",
    "记忆忍者"
};

const string ACHIEVEMENT_DESCS[ACH_COUNT] = {
    "完成第一次N-Back测试",
    "总体准确率超过90%",
    "平均响应时间小于1000毫秒",
    "获得100%准确率",
    "完成N=5的高难度测试",
    "参与多人挑战赛",
    "连续3次测试准确率超过80%",
    "完成总计10次测试",
    "视觉和听觉准确率都超过90%",
    "在N=3难度下获得85%以上准确率"
};

// 玩家统计
struct PlayerStats {
    string name;
    int totalTests;
    int totalTrials;
    int maxNLevel;
    double bestAccuracy;
    double bestResponseTime;
    int achievements[ACH_COUNT];
    vector<double> recentAccuracies;
    
    PlayerStats() : name(""), totalTests(0), totalTrials(0), maxNLevel(0),
                   bestAccuracy(0), bestResponseTime(10000) {
        memset(achievements, 0, sizeof(achievements));
    }
};

// 游戏统计
struct GameStats {
    string playerName;
    int nValue;
    int totalTrials;
    int visualHits;
    int visualFalseAlarms;
    int visualMisses;
    int visualCorrectRejections;
    int auditoryHits;
    int auditoryFalseAlarms;
    int auditoryMisses;
    int auditoryCorrectRejections;
    double visualAccuracy;
    double auditoryAccuracy;
    double overallAccuracy;
    double responseTimeAvg;
    
    GameStats() : playerName(""), nValue(0), totalTrials(0),
                 visualHits(0), visualFalseAlarms(0), visualMisses(0), visualCorrectRejections(0),
                 auditoryHits(0), auditoryFalseAlarms(0), auditoryMisses(0), auditoryCorrectRejections(0),
                 visualAccuracy(0), auditoryAccuracy(0), overallAccuracy(0), responseTimeAvg(0) {}
    
    void calculateAccuracies() {
        int visualTotal = visualHits + visualMisses + visualFalseAlarms + visualCorrectRejections;
        int auditoryTotal = auditoryHits + auditoryMisses + auditoryFalseAlarms + auditoryCorrectRejections;
        
        if (visualTotal > 0) {
            visualAccuracy = (visualHits + visualCorrectRejections) * 100.0 / visualTotal;
        }
        
        if (auditoryTotal > 0) {
            auditoryAccuracy = (auditoryHits + auditoryCorrectRejections) * 100.0 / auditoryTotal;
        }
        
        if (visualTotal + auditoryTotal > 0) {
            overallAccuracy = (visualHits + visualCorrectRejections + 
                              auditoryHits + auditoryCorrectRejections) * 100.0 / 
                              (visualTotal + auditoryTotal);
        }
    }
};

// 玩家信息
struct Player {
    string name;
    GameStats currentStats;
    PlayerStats careerStats;
    bool isActive;
};

// 成就系统类
class AchievementSystem {
private:
    map<string, PlayerStats> allPlayers;
    
public:
    void loadPlayerStats() {
        ifstream inFile("player_stats.dat", ios::binary);
        if (!inFile) return;
        
        while (!inFile.eof()) {
            PlayerStats stats;
            size_t nameLen;
            
            inFile.read((char*)&nameLen, sizeof(nameLen));
            if (inFile.eof()) break;
            
            char* nameBuf = new char[nameLen + 1];
            inFile.read(nameBuf, nameLen);
            nameBuf[nameLen] = '\0';
            stats.name = nameBuf;
            delete[] nameBuf;
            
            inFile.read((char*)&stats.totalTests, sizeof(stats.totalTests));
            inFile.read((char*)&stats.totalTrials, sizeof(stats.totalTrials));
            inFile.read((char*)&stats.maxNLevel, sizeof(stats.maxNLevel));
            inFile.read((char*)&stats.bestAccuracy, sizeof(stats.bestAccuracy));
            inFile.read((char*)&stats.bestResponseTime, sizeof(stats.bestResponseTime));
            inFile.read((char*)stats.achievements, sizeof(stats.achievements));
            
            allPlayers[stats.name] = stats;
        }
        
        inFile.close();
    }
    
    void savePlayerStats() {
        ofstream outFile("player_stats.dat", ios::binary);
        if (!outFile) return;
        
        for (const auto& pair : allPlayers) {
            const PlayerStats& stats = pair.second;
            size_t nameLen = stats.name.length();
            
            outFile.write((const char*)&nameLen, sizeof(nameLen));
            outFile.write(stats.name.c_str(), nameLen);
            outFile.write((const char*)&stats.totalTests, sizeof(stats.totalTests));
            outFile.write((const char*)&stats.totalTrials, sizeof(stats.totalTrials));
            outFile.write((const char*)&stats.maxNLevel, sizeof(stats.maxNLevel));
            outFile.write((const char*)&stats.bestAccuracy, sizeof(stats.bestAccuracy));
            outFile.write((const char*)&stats.bestResponseTime, sizeof(stats.bestResponseTime));
            outFile.write((const char*)stats.achievements, sizeof(stats.achievements));
        }
        
        outFile.close();
    }
    
    PlayerStats* getPlayerStats(const string& name) {
        if (allPlayers.find(name) == allPlayers.end()) {
            PlayerStats newStats;
            newStats.name = name;
            allPlayers[name] = newStats;
        }
        return &allPlayers[name];
    }
    
    vector<string> checkAchievements(PlayerStats& stats, const GameStats& gameStats) {
        vector<string> newAchievements;
        
        // 成就1: 初学者
        if (stats.totalTests == 1 && stats.achievements[ACH_NOVICE] == 0) {
            stats.achievements[ACH_NOVICE] = 1;
            newAchievements.push_back(ACHIEVEMENT_NAMES[ACH_NOVICE]);
        }
        
        // 成就2: 记忆大师
        if (gameStats.overallAccuracy >= 90.0 && stats.achievements[ACH_MEMORY_MASTER] == 0) {
            stats.achievements[ACH_MEMORY_MASTER] = 1;
            newAchievements.push_back(ACHIEVEMENT_NAMES[ACH_MEMORY_MASTER]);
        }
        
        // 成就3: 快速思考者
        if (gameStats.responseTimeAvg <= 1000.0 && stats.achievements[ACH_FAST_THINKER] == 0) {
            stats.achievements[ACH_FAST_THINKER] = 1;
            newAchievements.push_back(ACHIEVEMENT_NAMES[ACH_FAST_THINKER]);
        }
        
        // 成就4: 完美分数
        if (gameStats.overallAccuracy >= 99.9 && stats.achievements[ACH_PERFECT_SCORE] == 0) {
            stats.achievements[ACH_PERFECT_SCORE] = 1;
            newAchievements.push_back(ACHIEVEMENT_NAMES[ACH_PERFECT_SCORE]);
        }
        
        // 成就5: 挑战者
        if (gameStats.nValue >= 5 && stats.achievements[ACH_CHALLENGER] == 0) {
            stats.achievements[ACH_CHALLENGER] = 1;
            newAchievements.push_back(ACHIEVEMENT_NAMES[ACH_CHALLENGER]);
        }
        
        // 成就8: 测试达人
        if (stats.totalTests >= 10 && stats.achievements[ACH_TOTAL_TESTS] == 0) {
            stats.achievements[ACH_TOTAL_TESTS] = 1;
            newAchievements.push_back(ACHIEVEMENT_NAMES[ACH_TOTAL_TESTS]);
        }
        
        // 成就9: 双项专家
        if (gameStats.visualAccuracy >= 90.0 && gameStats.auditoryAccuracy >= 90.0 && 
            stats.achievements[ACH_DUAL_EXPERT] == 0) {
            stats.achievements[ACH_DUAL_EXPERT] = 1;
            newAchievements.push_back(ACHIEVEMENT_NAMES[ACH_DUAL_EXPERT]);
        }
        
        // 成就10: 记忆忍者
        if (gameStats.nValue >= 3 && gameStats.overallAccuracy >= 85.0 && 
            stats.achievements[ACH_NINJA] == 0) {
            stats.achievements[ACH_NINJA] = 1;
            newAchievements.push_back(ACHIEVEMENT_NAMES[ACH_NINJA]);
        }
        
        return newAchievements;
    }
    
    void updatePlayerStats(PlayerStats& stats, const GameStats& gameStats) {
        stats.totalTests++;
        stats.totalTrials += gameStats.totalTrials;
        
        if (gameStats.nValue > stats.maxNLevel) {
            stats.maxNLevel = gameStats.nValue;
        }
        
        if (gameStats.overallAccuracy > stats.bestAccuracy) {
            stats.bestAccuracy = gameStats.overallAccuracy;
        }
        
        if (gameStats.responseTimeAvg < stats.bestResponseTime) {
            stats.bestResponseTime = gameStats.responseTimeAvg;
        }
        
        stats.recentAccuracies.push_back(gameStats.overallAccuracy);
        if (stats.recentAccuracies.size() > 3) {
            stats.recentAccuracies.erase(stats.recentAccuracies.begin());
        }
        
        // 成就6: 稳定发挥
        if (stats.recentAccuracies.size() == 3) {
            bool allOver80 = true;
            for (double acc : stats.recentAccuracies) {
                if (acc < 80.0) {
                    allOver80 = false;
                    break;
                }
            }
            if (allOver80 && stats.achievements[ACH_CONSISTENT] == 0) {
                stats.achievements[ACH_CONSISTENT] = 1;
            }
        }
        
        savePlayerStats();
    }
    
    void displayPlayerAchievements(const string& playerName) {
        PlayerStats* stats = getPlayerStats(playerName);
        
        clearScreen();
        cout << "========================================\n";
        cout << "      " << playerName << " 的成就系统\n";
        cout << "========================================\n";
        
        cout << "\n统计信息:\n";
        cout << "总测试次数: " << stats->totalTests << "\n";
        cout << "总试次数量: " << stats->totalTrials << "\n";
        cout << "最高N值: " << stats->maxNLevel << "\n";
        cout << "最佳准确率: " << fixed << setprecision(1) << stats->bestAccuracy << "%\n";
        cout << "最佳响应时间: " << fixed << setprecision(0) << stats->bestResponseTime << "ms\n";
        
        cout << "\n已获得成就 (" << getAchievementCount(*stats) << "/" << ACH_COUNT-1 << "):\n";
        cout << "----------------------------------------\n";
        
        int count = 0;
        for (int i = 0; i < ACH_COUNT-1; i++) {
            if (stats->achievements[i]) {
                cout << "[V] " << ACHIEVEMENT_NAMES[i] << "\n";
                cout << "    " << ACHIEVEMENT_DESCS[i] << "\n\n";
                count++;
            }
        }
        
        cout << "\n未获得成就:\n";
        cout << "----------------------------------------\n";
        for (int i = 0; i < ACH_COUNT-1; i++) {
            if (!stats->achievements[i]) {
                cout << "[ ] " << ACHIEVEMENT_NAMES[i] << "\n";
                cout << "    " << ACHIEVEMENT_DESCS[i] << "\n\n";
            }
        }
        
        cout << "========================================\n";
        cout << "按任意键返回...";
        cin.ignore();
        cin.get();
    }
    
    int getAchievementCount(const PlayerStats& stats) {
        int count = 0;
        for (int i = 0; i < ACH_COUNT-1; i++) {
            if (stats.achievements[i]) count++;
        }
        return count;
    }
};

// 网络通信类
class NetworkManager {
private:
#ifdef _WIN32
    SOCKET sock;
    WSADATA wsaData;
#else
    int sock;
#endif
    bool isConnected;
    
public:
    NetworkManager() : isConnected(false) {
#ifdef _WIN32
        if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
            cout << "WSAStartup failed!\n";
        }
#endif
    }
    
    ~NetworkManager() {
        disconnect();
#ifdef _WIN32
        WSACleanup();
#endif
    }
    
    bool startServer(int port) {
#ifdef _WIN32
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) return false;
        
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port);
        
        if (bind(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            closesocket(sock);
            return false;
        }
        
        listen(sock, 5);
        cout << "服务器已启动，监听端口 " << port << "...\n";
#else
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) return false;
        
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port);
        
        if (bind(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            close(sock);
            return false;
        }
        
        listen(sock, 5);
        cout << "Server started on port " << port << "...\n";
#endif
        isConnected = true;
        return true;
    }
    
    bool connectToServer(const string& ip, int port) {
#ifdef _WIN32
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) return false;
        
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());
        
        if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            closesocket(sock);
            return false;
        }
#else
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) return false;
        
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr);
        
        if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            close(sock);
            return false;
        }
#endif
        isConnected = true;
        cout << "已连接到服务器 " << ip << ":" << port << "\n";
        return true;
    }
    
    void disconnect() {
        if (isConnected) {
#ifdef _WIN32
            closesocket(sock);
#else
            close(sock);
#endif
            isConnected = false;
        }
    }
    
    bool sendData(const string& data) {
        if (!isConnected) return false;
        
        string sizeStr = to_string(data.length()) + "\n";
#ifdef _WIN32
        send(sock, sizeStr.c_str(), sizeStr.length(), 0);
        send(sock, data.c_str(), data.length(), 0);
        return true;
#else
        write(sock, sizeStr.c_str(), sizeStr.length());
        write(sock, data.c_str(), data.length());
        return true;
#endif
    }
    
    string receiveData() {
        if (!isConnected) return "";
        
        char buffer[1024];
        string data;
        
#ifdef _WIN32
        int bytesReceived = recv(sock, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            data = buffer;
        }
#else
        int bytesReceived = read(sock, buffer, sizeof(buffer)-1);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            data = buffer;
        }
#endif
        return data;
    }
    
    bool isReady() const { return isConnected; }
};

class NBackGame {
private:
    int n;
    int totalTrials;
    int currentTrial;
    vector<Stimulus> stimulusHistory;
    int gridSize;
    
    int stimulusDuration;
    int interStimulusInterval;
    
    vector<Player> players;
    vector<Stimulus> predefinedStimuli;
    bool usePredefinedSequence;
    
    AchievementSystem achievementSys;
    NetworkManager network;
    bool isServer;
    
public:
    NBackGame(int nValue, int trials, int stimDuration = 2000, int isi = 500)
        : n(nValue), totalTrials(trials), currentTrial(0), gridSize(3),
          stimulusDuration(stimDuration), interStimulusInterval(isi),
          usePredefinedSequence(false), isServer(false) {
        srand(static_cast<unsigned>(time(nullptr)));
        achievementSys.loadPlayerStats();
    }
    
    void addPlayer(const string& name) {
        Player player;
        player.name = name;
        player.isActive = true;
        
        // 加载玩家生涯数据
        PlayerStats* stats = achievementSys.getPlayerStats(name);
        player.careerStats = *stats;
        
        players.push_back(player);
    }
    
    void generatePredefinedSequence() {
        predefinedStimuli.clear();
        stimulusHistory.clear();
        
        for (int i = 0; i < totalTrials; i++) {
            Stimulus stim = generateStimulus(i);
            predefinedStimuli.push_back(stim);
            stimulusHistory.push_back(stim);
        }
        
        usePredefinedSequence = true;
        stimulusHistory.clear();
    }
    
    Stimulus generateStimulus(int trialIndex) {
        if (usePredefinedSequence && trialIndex < predefinedStimuli.size()) {
            return predefinedStimuli[trialIndex];
        }
        
        Stimulus stim;
        
        bool shouldMatchVisual = (trialIndex >= n) && (rand() % 100 < 30);
        bool shouldMatchAuditory = (trialIndex >= n) && (rand() % 100 < 30);
        
        if (shouldMatchVisual && trialIndex >= n) {
            stim.visualPosition = stimulusHistory[trialIndex - n].visualPosition;
        } else {
            int attempts = 0;
            do {
                stim.visualPosition = rand() % (gridSize * gridSize);
                attempts++;
            } while (trialIndex >= n && 
                    stim.visualPosition == stimulusHistory[trialIndex - n].visualPosition &&
                    attempts < 10);
        }
        
        if (shouldMatchAuditory && trialIndex >= n) {
            stim.auditoryLetter = stimulusHistory[trialIndex - n].auditoryLetter;
        } else {
            int attempts = 0;
            do {
                stim.auditoryLetter = 'A' + rand() % 26;
                attempts++;
            } while (trialIndex >= n && 
                    stim.auditoryLetter == stimulusHistory[trialIndex - n].auditoryLetter &&
                    attempts < 10);
        }
        
        return stim;
    }
    
    void displayGrid(const Stimulus& stim, const string& currentPlayer = "") {
        cout << "\n";
        if (!currentPlayer.empty()) {
            cout << "当前玩家: " << currentPlayer << "\n";
        }
        cout << "试次: " << (currentTrial + 1) << "/" << totalTrials << "  (N=" << n << ")\n";
        cout << "+-----+-----+-----+\n";
        
        for (int row = 0; row < gridSize; row++) {
            cout << "|";
            for (int col = 0; col < gridSize; col++) {
                int pos = row * gridSize + col;
                if (pos == stim.visualPosition) {
                    cout << "  #  |";
                } else {
                    cout << "  .  |";
                }
            }
            cout << "\n";
            
            if (row < gridSize - 1) {
                cout << "+-----+-----+-----+\n";
            }
        }
        
        cout << "+-----+-----+-----+\n";
    }
    
    // 远程游戏：启动服务器
    bool startRemoteServer(int port = 8888) {
        isServer = true;
        return network.startServer(port);
    }
    
    // 远程游戏：连接到服务器
    bool connectToRemoteServer(const string& ip = "127.0.0.1", int port = 8888) {
        isServer = false;
        return network.connectToServer(ip, port);
    }
    
    // 同步游戏设置到所有客户端
    void syncGameSettings() {
        if (!network.isReady()) return;
        
        stringstream ss;
        ss << "SETTINGS:" << n << ":" << totalTrials;
        network.sendData(ss.str());
    }
    
    // 接收远程数据
    void processRemoteData() {
        if (!network.isReady()) return;
        
        string data = network.receiveData();
        if (!data.empty()) {
            cout << "收到远程数据: " << data << "\n";
            // 处理接收到的游戏数据
        }
    }
    
    GameStats runSinglePlayerTest(Player& player, int playerIndex) {
        clearScreen();
        cout << "=== 玩家 " << (playerIndex + 1) << ": " << player.name << " ===\n";
        cout << "准备开始测试，按任意键继续...";
        cin.ignore();
        cin.get();
        
        player.currentStats = GameStats();
        player.currentStats.playerName = player.name;
        player.currentStats.nValue = n;
        player.currentStats.totalTrials = totalTrials;
        
        stimulusHistory.clear();
        
        // 如果是客户端，同步设置
        if (!isServer && network.isReady()) {
            syncGameSettings();
        }
        
        for (int i = 0; i < totalTrials; i++) {
            currentTrial = i;
            
            Stimulus currentStim = generateStimulus(i);
            stimulusHistory.push_back(currentStim);
            
            bool visualMatch = false;
            bool auditoryMatch = false;
            if (i >= n) {
                Stimulus nBackStim = stimulusHistory[i - n];
                visualMatch = (currentStim.visualPosition == nBackStim.visualPosition);
                auditoryMatch = (currentStim.auditoryLetter == nBackStim.auditoryLetter);
            }
            
            auto startTime = chrono::steady_clock::now();
            auto responses = presentStimulusAndGetResponse(currentStim, i, player.name);
            auto endTime = chrono::steady_clock::now();
            
            long responseTime = chrono::duration_cast<chrono::milliseconds>(
                endTime - startTime).count();
            
            bool userVisual = responses.first;
            bool userAuditory = responses.second;
            
            updatePlayerStats(player.currentStats, visualMatch, auditoryMatch, 
                             userVisual, userAuditory, responseTime);
            
            displayFeedback(i, visualMatch, auditoryMatch, userVisual, userAuditory);
            
            this_thread::sleep_for(chrono::milliseconds(500));
        }
        
        player.currentStats.calculateAccuracies();
        
        // 更新成就系统
        achievementSys.updatePlayerStats(player.careerStats, player.currentStats);
        vector<string> newAchievements = achievementSys.checkAchievements(player.careerStats, player.currentStats);
        
        showPlayerResults(player.currentStats, newAchievements);
        
        cout << "\n按任意键继续...";
        cin.ignore();
        cin.get();
        
        return player.currentStats;
    }
    
    pair<bool, bool> presentStimulusAndGetResponse(const Stimulus& stim, int trialIndex, 
                                                   const string& playerName) {
        clearScreen();
        cout << "=== 玩家: " << playerName << " ===\n";
        
        displayGrid(stim, playerName);
        
        cout << "\n听觉刺激: " << stim.auditoryLetter << "\n";
        cout << "\n";
        
        cout << "提示:\n";
        cout << "  - 视觉匹配(N=" << n << "步前): 按 'V' 键\n";
        cout << "  - 听觉匹配(N=" << n << "步前): 按 'A' 键\n";
        cout << "  - 同时匹配: 两个键都按\n";
        cout << "\n";
        
        if (trialIndex < n) {
            cout << "注意: 前 " << n << " 次刺激没有参照，无需按键!\n";
        }
        
        bool userVisualResponse = false;
        bool userAuditoryResponse = false;
        
        auto startTime = chrono::steady_clock::now();
        while (chrono::duration_cast<chrono::milliseconds>(
                chrono::steady_clock::now() - startTime).count() < stimulusDuration) {
            
            char key = getKeyInput();
            if (key == 'v' || key == 'V') {
                userVisualResponse = true;
            }
            if (key == 'a' || key == 'A') {
                userAuditoryResponse = true;
            }
            
            auto elapsed = chrono::duration_cast<chrono::milliseconds>(
                chrono::steady_clock::now() - startTime).count();
            int remaining = stimulusDuration - elapsed;
            
            cout << "\r剩余时间: " << setw(4) << remaining << " ms  ";
            if (userVisualResponse) cout << " [V]";
            if (userAuditoryResponse) cout << " [A]";
            cout << "      ";
            
            this_thread::sleep_for(chrono::milliseconds(50));
        }
        
#ifdef _WIN32
        while (_kbhit()) _getch();
#else
        tcflush(STDIN_FILENO, TCIFLUSH);
#endif
        
        cout << "\n";
        return {userVisualResponse, userAuditoryResponse};
    }
    
    void updatePlayerStats(GameStats& stats, bool visualMatch, bool auditoryMatch, 
                          bool userVisual, bool userAuditory, long responseTime) {
        stats.totalTrials++;
        
        if (visualMatch && userVisual) stats.visualHits++;
        else if (visualMatch && !userVisual) stats.visualMisses++;
        else if (!visualMatch && userVisual) stats.visualFalseAlarms++;
        else if (!visualMatch && !userVisual) stats.visualCorrectRejections++;
        
        if (auditoryMatch && userAuditory) stats.auditoryHits++;
        else if (auditoryMatch && !userAuditory) stats.auditoryMisses++;
        else if (!auditoryMatch && userAuditory) stats.auditoryFalseAlarms++;
        else if (!auditoryMatch && !userAuditory) stats.auditoryCorrectRejections++;
        
        if (stats.totalTrials > 1) {
            stats.responseTimeAvg = (stats.responseTimeAvg * (stats.totalTrials - 1) + responseTime) / stats.totalTrials;
        } else {
            stats.responseTimeAvg = responseTime;
        }
    }
    
    void displayFeedback(int trialIndex, bool visualMatch, bool auditoryMatch,
                         bool userVisual, bool userAuditory) {
        clearScreen();
        cout << "=== 结果反馈 ===\n";
        cout << "试次: " << (trialIndex + 1) << "/" << totalTrials << "\n";
        
        if (trialIndex >= n) {
            cout << "\n实际匹配情况:\n";
            cout << "  视觉: " << (visualMatch ? "匹配 [V]" : "不匹配 [X]") << "\n";
            cout << "  听觉: " << (auditoryMatch ? "匹配 [V]" : "不匹配 [X]") << "\n";
            
            cout << "\n你的响应:\n";
            cout << "  视觉: " << (userVisual ? "是 [V]" : "否 [X]") << "\n";
            cout << "  听觉: " << (userAuditory ? "是 [V]" : "否 [X]") << "\n";
            
            cout << "\n结果:\n";
            bool visualCorrect = (visualMatch == userVisual);
            bool auditoryCorrect = (auditoryMatch == userAuditory);
            
            cout << "  视觉: " << (visualCorrect ? "正确! [V]" : "错误! [X]") << "\n";
            cout << "  听觉: " << (auditoryCorrect ? "正确! [V]" : "错误! [X]") << "\n";
            
            if (visualCorrect && auditoryCorrect) {
                cout << "\n优秀! 双项正确!\n";
            } else if (visualCorrect || auditoryCorrect) {
                cout << "\n不错! 一项正确!\n";
            } else {
                cout << "\n继续努力!\n";
            }
        } else {
            cout << "\n(前 " << n << " 次刺激是热身，不计分)\n";
        }
        
        cout << "\n下一个刺激将在 1 秒后出现...\n";
        this_thread::sleep_for(chrono::milliseconds(1000));
    }
    
    void showPlayerResults(const GameStats& stats, const vector<string>& newAchievements) {
        clearScreen();
        cout << "========================================\n";
        cout << "       " << stats.playerName << " 的个人成绩\n";
        cout << "========================================\n";
        cout << "N值: " << stats.nValue << "\n";
        cout << "总试次: " << stats.totalTrials << "\n";
        cout << "平均响应时间: " << fixed << setprecision(0) << stats.responseTimeAvg << " ms\n";
        cout << "\n";
        
        // 显示新获得的成就
        if (!newAchievements.empty()) {
            cout << "=== 新获得成就 ===\n";
            for (const string& ach : newAchievements) {
                cout << ">> " << ach << " <<\n";
            }
            cout << "\n";
        }
        
        int visualTotalResponses = stats.visualHits + stats.visualMisses + 
                                  stats.visualFalseAlarms + stats.visualCorrectRejections;
        int auditoryTotalResponses = stats.auditoryHits + stats.auditoryMisses + 
                                    stats.auditoryFalseAlarms + stats.auditoryCorrectRejections;
        
        if (visualTotalResponses > 0) {
            cout << "=== 视觉任务 ===\n";
            cout << "命中: " << stats.visualHits << "\n";
            cout << "漏报: " << stats.visualMisses << "\n";
            cout << "虚报: " << stats.visualFalseAlarms << "\n";
            cout << "正确拒绝: " << stats.visualCorrectRejections << "\n";
            
            double visualHitRate = (stats.visualHits + stats.visualMisses > 0) ?
                stats.visualHits * 100.0 / (stats.visualHits + stats.visualMisses) : 0;
            
            cout << fixed << setprecision(1);
            cout << "视觉准确率: " << stats.visualAccuracy << "%\n";
            cout << "视觉命中率: " << visualHitRate << "%\n";
        }
        
        if (auditoryTotalResponses > 0) {
            cout << "\n=== 听觉任务 ===\n";
            cout << "命中: " << stats.auditoryHits << "\n";
            cout << "漏报: " << stats.auditoryMisses << "\n";
            cout << "虚报: " << stats.auditoryFalseAlarms << "\n";
            cout << "正确拒绝: " << stats.auditoryCorrectRejections << "\n";
            
            double auditoryHitRate = (stats.auditoryHits + stats.auditoryMisses > 0) ?
                stats.auditoryHits * 100.0 / (stats.auditoryHits + stats.auditoryMisses) : 0;
            
            cout << "听觉准确率: " << stats.auditoryAccuracy << "%\n";
            cout << "听觉命中率: " << auditoryHitRate << "%\n";
        }
        
        cout << "\n=== 总体表现 ===\n";
        cout << "总体准确率: " << stats.overallAccuracy << "%\n";
        
        if (stats.overallAccuracy > 90) {
            cout << "评价: 记忆大师!\n";
        } else if (stats.overallAccuracy > 80) {
            cout << "评价: 优秀!\n";
        } else if (stats.overallAccuracy > 70) {
            cout << "评价: 良好!\n";
        } else if (stats.overallAccuracy > 60) {
            cout << "评价: 合格!\n";
        } else {
            cout << "评价: 需要更多练习!\n";
        }
        
        cout << "========================================\n";
    }
    
    void runMultiplayerGame() {
        if (players.empty()) {
            cout << "没有添加玩家！\n";
            return;
        }
        
        clearScreen();
        cout << "=== 多人 N-Back 挑战赛 ===\n";
        cout << "玩家列表 (" << players.size() << "人):\n";
        for (size_t i = 0; i < players.size(); i++) {
            cout << "  " << (i + 1) << ". " << players[i].name << "\n";
        }
        cout << "\nN值: " << n << "  试次: " << totalTrials << "\n";
        cout << "\n所有玩家将使用相同的题目进行测试！\n";
        cout << "\n按任意键开始测试...";
        cin.ignore();
        cin.get();
        
        // 更新成就：多人游戏
        for (Player& player : players) {
            if (player.careerStats.achievements[ACH_MULTIPLAYER] == 0) {
                player.careerStats.achievements[ACH_MULTIPLAYER] = 1;
            }
        }
        
        generatePredefinedSequence();
        
        vector<GameStats> allStats;
        for (size_t i = 0; i < players.size(); i++) {
            GameStats stats = runSinglePlayerTest(players[i], i);
            allStats.push_back(stats);
        }
        
        showLeaderboard(allStats);
        saveResultsToFile(allStats);
    }
    
    void showLeaderboard(const vector<GameStats>& allStats) {
        clearScreen();
        cout << "========================================\n";
        cout << "       N-Back 挑战赛排行榜\n";
        cout << "========================================\n";
        cout << "N值: " << n << "  总试次: " << totalTrials << "\n";
        cout << "玩家数量: " << players.size() << "\n\n";
        
        vector<GameStats> sortedStats = allStats;
        sort(sortedStats.begin(), sortedStats.end(), 
             [](const GameStats& a, const GameStats& b) {
                 return a.overallAccuracy > b.overallAccuracy;
             });
        
        cout << "+-----+--------------------+------------+------------+------------+------------+\n";
        cout << "| 排名 |       玩家        | 总体准确率 | 视觉准确率 | 听觉准确率 | 响应时间(ms) |\n";
        cout << "+-----+--------------------+------------+------------+------------+------------+\n";
        
        for (size_t i = 0; i < sortedStats.size(); i++) {
            const GameStats& stats = sortedStats[i];
            
            string medal;
            if (i == 0) medal = "1st";
            else if (i == 1) medal = "2nd";
            else if (i == 2) medal = "3rd";
            else medal = to_string(i + 1);
            
            cout << "| " << setw(3) << medal << " | "
                 << setw(18) << left << stats.playerName << " | "
                 << setw(10) << right << fixed << setprecision(1) << stats.overallAccuracy << "% | "
                 << setw(10) << right << stats.visualAccuracy << "% | "
                 << setw(10) << right << stats.auditoryAccuracy << "% | "
                 << setw(10) << right << setprecision(0) << stats.responseTimeAvg << " |\n";
        }
        
        cout << "+-----+--------------------+------------+------------+------------+------------+\n";
        
        if (!sortedStats.empty()) {
            cout << "\n冠军: " << sortedStats[0].playerName 
                 << " (" << fixed << setprecision(1) << sortedStats[0].overallAccuracy << "%)\n";
            
            cout << "\n冠军分析:\n";
            if (sortedStats[0].visualAccuracy > sortedStats[0].auditoryAccuracy) {
                cout << "  - 视觉记忆更强 (领先" 
                     << fixed << setprecision(1) 
                     << (sortedStats[0].visualAccuracy - sortedStats[0].auditoryAccuracy) 
                     << "%)\n";
            } else if (sortedStats[0].auditoryAccuracy > sortedStats[0].visualAccuracy) {
                cout << "  - 听觉记忆更强 (领先" 
                     << fixed << setprecision(1) 
                     << (sortedStats[0].auditoryAccuracy - sortedStats[0].visualAccuracy) 
                     << "%)\n";
            } else {
                cout << "  - 视觉和听觉记忆均衡发展\n";
            }
            
            cout << "  - 平均响应时间: " << fixed << setprecision(0) 
                 << sortedStats[0].responseTimeAvg << " ms\n";
        }
        
        cout << "\n训练建议:\n";
        if (n <= 2) {
            cout << "  - 当前N值较低，适合初学者，建议逐渐增加难度\n";
        } else if (n <= 4) {
            cout << "  - 当前N值适中，适合有一定经验的玩家\n";
        } else {
            cout << "  - 当前N值较高，挑战性强，适合记忆高手\n";
        }
        
        cout << "\n按任意键返回主菜单...";
        cin.ignore();
        cin.get();
    }
    
    void saveResultsToFile(const vector<GameStats>& allStats) {
        ofstream outFile("nback_results.txt", ios::app);
        if (!outFile) return;
        
        time_t now = time(nullptr);
        char timeStr[100];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
        
        outFile << "\n========================================\n";
        outFile << "测试时间: " << timeStr << "\n";
        outFile << "N值: " << n << "  试次: " << totalTrials << "\n";
        outFile << "玩家数量: " << players.size() << "\n\n";
        
        vector<GameStats> sortedStats = allStats;
        sort(sortedStats.begin(), sortedStats.end(), 
             [](const GameStats& a, const GameStats& b) {
                 return a.overallAccuracy > b.overallAccuracy;
             });
        
        for (size_t i = 0; i < sortedStats.size(); i++) {
            const GameStats& stats = sortedStats[i];
            outFile << "排名 " << (i + 1) << ": " << stats.playerName << "\n";
            outFile << "  总体准确率: " << fixed << setprecision(1) << stats.overallAccuracy << "%\n";
            outFile << "  视觉准确率: " << stats.visualAccuracy << "%\n";
            outFile << "  听觉准确率: " << stats.auditoryAccuracy << "%\n";
            outFile << "  响应时间: " << setprecision(0) << stats.responseTimeAvg << " ms\n";
            outFile << "  视觉: 命中" << stats.visualHits << "/漏报" << stats.visualMisses 
                   << "/虚报" << stats.visualFalseAlarms << "\n";
            outFile << "  听觉: 命中" << stats.auditoryHits << "/漏报" << stats.auditoryMisses 
                   << "/虚报" << stats.auditoryFalseAlarms << "\n\n";
        }
        
        outFile.close();
        cout << "\n成绩已保存到 nback_results.txt 文件\n";
    }
    
    // 显示成就系统
    void showAchievementSystem(const string& playerName) {
        achievementSys.displayPlayerAchievements(playerName);
    }
};

// 远程游戏菜单
void remoteGameMenu() {
    clearScreen();
    cout << "========================================\n";
    cout << "           远程联机游戏\n";
    cout << "========================================\n";
    cout << "1. 创建房间(作为主机)\n";
    cout << "2. 加入房间(作为客户端)\n";
    cout << "3. 返回主菜单\n";
    cout << "========================================\n";
    cout << "请选择 (1-3): ";
    
    int choice;
    cin >> choice;
    
    if (choice == 3) return;
    
    if (choice == 1) {
        int port;
        cout << "请输入端口号 (默认8888): ";
        cin >> port;
        if (port <= 0) port = 8888;
        
        NBackGame game(2, 20); // 默认参数
        if (game.startRemoteServer(port)) {
            cout << "房间创建成功！等待玩家加入...\n";
            cout << "按任意键开始游戏...";
            cin.ignore();
            cin.get();
            
            // 添加主机玩家
            string hostName;
            cout << "请输入你的名字: ";
            cin >> hostName;
            game.addPlayer(hostName);
            
            // 等待其他玩家加入
            cout << "等待其他玩家加入...\n";
            // 这里可以添加等待逻辑
            
            game.runMultiplayerGame();
        } else {
            cout << "房间创建失败！\n";
            cout << "按任意键返回...";
            cin.ignore();
            cin.get();
        }
    } else if (choice == 2) {
        string ip;
        int port;
        cout << "请输入服务器IP地址: ";
        cin >> ip;
        cout << "请输入端口号: ";
        cin >> port;
        
        NBackGame game(2, 20);
        if (game.connectToRemoteServer(ip, port)) {
            string playerName;
            cout << "请输入你的名字: ";
            cin >> playerName;
            game.addPlayer(playerName);
            
            cout << "等待主机开始游戏...\n";
            // 这里可以添加等待主机开始的逻辑
            
            // 单人游戏模式
            game.runSinglePlayerTest(*(new Player()), 0);
        } else {
            cout << "连接服务器失败！\n";
            cout << "按任意键返回...";
            cin.ignore();
            cin.get();
        }
    }
}

// 成就系统菜单
void achievementSystemMenu() {
    clearScreen();
    cout << "========================================\n";
    cout << "           成就系统\n";
    cout << "========================================\n";
    
    string playerName;
    cout << "请输入玩家名字: ";
    cin >> playerName;
    
    if (playerName.empty()) {
        cout << "玩家名不能为空！\n";
        cout << "按任意键返回...";
        cin.ignore();
        cin.get();
        return;
    }
    
    AchievementSystem achSys;
    achSys.loadPlayerStats();
    achSys.displayPlayerAchievements(playerName);
}

// 显示主菜单
void showMainMenu() {
    int choice = 0;
    AchievementSystem achSys;
    achSys.loadPlayerStats();
    
    while (true) {
        clearScreen();
        cout << "========================================\n";
        cout << "       N-Back 记忆训练系统 v3.0\n";
        cout << "========================================\n";
        cout << "1. 单人训练模式\n";
        cout << "2. 多人挑战赛模式\n";
        cout << "3. 远程联机游戏\n";
        cout << "4. 成就系统\n";
        cout << "5. 查看历史成绩\n";
        cout << "6. 关于 N-Back 训练\n";
        cout << "7. 退出系统\n";
        cout << "========================================\n";
        cout << "请选择 (1-7): ";
        
        cin >> choice;
        
        if (choice == 7) {
            cout << "再见! 坚持训练有助于提升记忆力!\n";
            break;
        }
        
        switch (choice) {
            case 1: {
                int nValue, trials;
                cout << "请输入 N 值 (推荐从 2 开始): ";
                cin >> nValue;
                cout << "请输入试次数量 (建议 20-40): ";
                cin >> trials;
                
                NBackGame game(nValue, trials);
                string playerName;
                cout << "请输入你的名字: ";
                cin >> playerName;
                game.addPlayer(playerName);
                game.runMultiplayerGame();
                break;
            }
            case 2: {
                int nValue, trials, playerCount;
                cout << "请输入 N 值: ";
                cin >> nValue;
                cout << "请输入试次数量: ";
                cin >> trials;
                cout << "请输入玩家数量 (2-10): ";
                cin >> playerCount;
                
                if (playerCount < 2 || playerCount > 10) {
                    cout << "玩家数量必须在 2-10 之间！\n";
                    break;
                }
                
                cin.ignore();
                NBackGame game(nValue, trials);
                
                for (int i = 0; i < playerCount; i++) {
                    string playerName;
                    cout << "请输入第 " << (i + 1) << " 位玩家的名字: ";
                    getline(cin, playerName);
                    
                    if (playerName.empty()) {
                        playerName = "玩家" + to_string(i + 1);
                    }
                    
                    game.addPlayer(playerName);
                }
                
                game.runMultiplayerGame();
                break;
            }
            case 3: {
                remoteGameMenu();
                break;
            }
            case 4: {
                achievementSystemMenu();
                break;
            }
            case 5: {
                ifstream inFile("nback_results.txt");
                if (!inFile) {
                    cout << "暂无历史成绩记录。\n";
                } else {
                    string line;
                    while (getline(inFile, line)) {
                        cout << line << "\n";
                    }
                    inFile.close();
                }
                cout << "\n按任意键返回...";
                cin.ignore();
                cin.get();
                break;
            }
            case 6: {
                clearScreen();
                cout << "========================================\n";
                cout << "         关于 N-Back 训练\n";
                cout << "========================================\n";
                cout << "\nN-Back 是一种工作记忆训练任务:\n";
                cout << "1. 你需要记住 N 步前的刺激\n";
                cout << "2. 如果当前刺激与 N 步前相同，做出响应\n";
                cout << "3. 双 N-Back 同时训练视觉和听觉工作记忆\n";
                cout << "\n新功能:\n";
                cout << "成就系统: 解锁各种记忆相关成就\n";
                cout << "远程联机: 与朋友在线比拼记忆力\n";
                cout << "生涯统计: 追踪你的进步历程\n";
                cout << "\n训练建议:\n";
                cout << "每天练习 15-20 分钟\n";
                cout << "从 N=2 开始，逐渐增加难度\n";
                cout << "保持专注，准确比速度更重要\n";
                cout << "\n按任意键返回...";
                cin.ignore();
                cin.get();
                break;
            }
            default:
                cout << "无效选择，请重新输入!\n";
                cin.ignore();
                cin.get();
                break;
        }
    }
}

int main() {
    srand(static_cast<unsigned>(time(nullptr)));
    
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    
    cout << "欢迎使用 N-Back 记忆训练系统 v3.0!\n";
    cout << "新增成就系统和远程联机功能！\n";
    cout << "按任意键继续...";
    cin.get();
    
    showMainMenu();
    
    return 0;
}
