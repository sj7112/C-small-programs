#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <chrono>
#include <map> // ����mapͷ�ļ�
#include <utility> // For std::pair
#include <algorithm> // For std::reverse

using namespace std;

// centos7 /etc/nginxĿ¼�£�ִ�У�g++ -std=c++11 -o blacklist blacklist.cpp

// ���Թ����ࣺ��ȡchar*ָ��λ�õ����� 
void readRangeFromBuffer(const char* buffer, size_t start, size_t end, const size_t BUFFER_SIZE = 4096) {
    // ȷ����ʼ����յ㲻�ᳬ������߽�
    if (start >= BUFFER_SIZE || end >= BUFFER_SIZE || start > end) {
        cerr << "Start or end position is out of bounds or invalid." << endl;
        return;
    }
    size_t length = end - start + 1; // ����ʵ�ʶ�ȡ�ĳ���
    string result(buffer + start, length); // ��ȡ���ַ���
    cout << result << endl; // ������
}

/**
 * ����ʱ�����ӵ�lines��ĩβ�������ļ������ȡ�� 
 * @param lines ����б� 
 * @param tempLine ����ӵ��ַ���
 * @param newLine true������һ�У�false����ӵ����һ��
 * @return
 */
void getlineReverse(vector<string>& lines, string& tempLine, bool newLine) {
	string str = string(tempLine.rbegin(), tempLine.rend()); // ��ת˳��
	tempLine.clear(); // �����ʱ���� 
	if (newLine) {
		lines.push_back(str); // ����һ�� 
	} else {
		lines[lines.size()-1] = str + lines[lines.size()-1]; // ��ӵ����һ��
	}
}

/**
 * ���ַ���תΪʱ�����nginxĬ����־�ļ���
 * @param timeStr ���ڸ�ʽ�����磺03/Sep/2024:14:05:03 +0800
 * @return time_t ʱ�������1970��1��1��00ʱ00��00�루��������ʱ�䣩�����ڵ������� 
 */
time_t parseTimeLogFile(const string& timeStr) {
	// �·���дת��Ϊ�·�������0 ~ 11��
    static unordered_map<string, int> monthMap = {
        {"Jan", 0}, {"Feb", 1}, {"Mar", 2}, {"Apr", 3},
        {"May", 4}, {"Jun", 5}, {"Jul", 6}, {"Aug", 7},
        {"Sep", 8}, {"Oct", 9}, {"Nov", 10}, {"Dec", 11}
    };
 
    // Parse the time string
    struct tm tm = {};
    istringstream ss(timeStr);
    
    // Manually parse the date and time part
    string dateTime;
    getline(ss, dateTime, ' ');  // Extract the date/time part before the space
    
    // Convert dateTime to tm structure
    string month; 
    stringstream dateTimeStream(dateTime);
    dateTimeStream >> setw(2) >> tm.tm_mday; // Day
    dateTimeStream.ignore(1); // Skip '/'
    dateTimeStream >> setw(3) >> month;
    tm.tm_mon = monthMap.find(month)->second; // Month
    dateTimeStream.ignore(1); // Skip '/'
    dateTimeStream >> setw(4) >> tm.tm_year; // Year
    tm.tm_year -= 1900; // Years since 1900
    
    char colon;
    dateTimeStream >> colon >> tm.tm_hour; // Hours
    dateTimeStream >> colon >> tm.tm_min; // Minutes
    dateTimeStream >> colon >> tm.tm_sec; // Seconds
    
    // Read timezone part and ignore
//    string timezone;
//    ss >> timezone;
    
    // Convert to time_t
    return mktime(&tm);
}

/**
 * �ļ�˳���ȡ�������ļ���˳���ȡ���е���
 * @param file �������ļ�
 * @return vector<string> ��ȡ����б�
 */
vector<string> readLines(ifstream& file) {
    vector<string> lines;

    file.seekg(0);
    string line;

    while (getline(file, line)) {
        lines.push_back(line);
    }
    // ��ȡ��ǰ�ļ�ָ��λ�ã�������ָ���к����һ����ʼλ��
    streampos currentPos = file.tellg();
    return lines;
}

/**
 * �ļ�˳���ȡ�������ļ�����ʼλ�ã�˳���ȡָ����������
 * @param file �������ļ� 
 * @param startPos ��ʼָ��λ�� 
 * @param numLines ��ȡ���� 
 * @return pair<vector<string>, streampos> ��ȡ����б� | ��ָ��λ�� 
 */
pair<vector<string>, streampos> readLines(ifstream& file, streampos startPos, size_t numLines) {
    vector<string> lines;

    file.seekg(startPos);
    string line;
    size_t count = 0;

    while (count < numLines && getline(file, line)) {
        lines.push_back(line);
        count++;
    }
    // ��ȡ��ǰ�ļ�ָ��λ�ã�������ָ���к����һ����ʼλ��
    streampos currentPos = file.tellg();
    return {lines, currentPos};
}

/**
 * �ļ������ȡ�������ļ��Ľ���λ�ã������ȡָ����������
 * @param file �������ļ� 
 * @param endPos ����ָ��λ�� 
 * @param numLines ��ȡ���� 
 * @return pair<vector<string>, streampos> ��ȡ����б� | ��ָ��λ�� 
 */
pair<vector<string>, streampos> readLinesReverse(ifstream& file, streampos endPos, size_t numLines) {
    const size_t BUFFER_SIZE = 4096; // ��ȡ�����ݿ��С
    char bufferArray[BUFFER_SIZE];
    vector<string> lines;
    streampos pos = endPos;
    size_t linesRead = 0; // �Ѷ����������ܻ��linesFinished��1�У� 
    size_t linesFinished = 0; // ��д��������\n��β�� 

    while (pos > 0 && linesFinished < numLines) {
        size_t bytesToRead = min(static_cast<streampos>(BUFFER_SIZE), pos); // 1 ~ pos���ܳ��� vs. 4096������ȡС
        pos -= bytesToRead; // pos��ǰ�ƶ�����ȡ�ַ�����
        file.seekg(pos); // �ļ������ȡ�ĳ�ʼ��λ
        file.read(bufferArray, bytesToRead); // һ���Զ�ȡ

        // �����ȡ������
        string tempLine; // ��ʱ�����ݣ�����д�룩 
        for (size_t i = bytesToRead; i > 0; --i) {
            if (bufferArray[i - 1] == '\n' || bufferArray[i - 1] == '\r') {
            	// ����windows��\r������������һ���ַ�\n��linuxΪ'\n'��windowsΪ'\r\n'�����������ɰ�MacOSΪ'\r'�� 
            	if (bufferArray[i - 1] == '\r' && i - 1 > 0 && bufferArray[i - 2] == '\n') continue; // \r\n�Զ�����\r 
                if (!tempLine.empty()) {
                    // �����ȡ��ÿ�ж���Ҫ�������
                    bool newLine = linesRead == linesFinished;
                	getlineReverse(lines, tempLine, newLine); 
                    if (newLine) ++linesRead; // �Ѷ�ȡ������ 						
                    ++linesFinished; // ����ɵ����� 
                }
                if (linesFinished >= numLines) {
                	pos += (i - 1); // ��ȥδ���������´δӴ˴����¶�ȡ�� 
                    break; // һ������ȡ�����У�Ĭ�� = 1000�� 
                }
            } else {
                tempLine += bufferArray[i - 1]; // �ַ�����װ 
            }
        }
        // ������û���ҵ����з�������Ҫ����ʣ�����
        if (linesRead < numLines && !tempLine.empty()) {
            bool newLine = linesRead == linesFinished; // ���⴦��һ�п���Զ����4096�ֽ� 
        	getlineReverse(lines, tempLine, newLine);
            ++linesRead;
        }
    }
    // ��ת������Ա���ļ�ĩβ���ļ���ͷ��˳��
    reverse(lines.begin(), lines.end());
    return make_pair(lines, pos);
}

// ����ṹ��
struct IPRecord {
    string ipAddress;  // IP ��ַ
    time_t opTime;  // ʱ���
    unsigned int count; // ������
};

/**
 * ��һ��stringתΪIPRecord��ʱ��Ϊʱ�����ʽ 
 */
IPRecord getLineIpRecord(string line) {
    IPRecord record;
    istringstream iss(line);
    // ������������ȡIP��ַ��ʱ����ͷ��ʴ���
    iss >> record.ipAddress >> record.opTime >> record.count;
    return record;
}

// �����60���ڷ��ʴ�����࣬�ҳ����������������޵�IP��ַ���������idx�� 
vector<IPRecord> getIpList(string LOG_FILE, const int timecount) {
    vector<IPRecord> ipList; // ���飬��ʽ����{"192.168.1.1", "03/Sep/2024:14:05:16 +0800", 5} 

    // ��ȡ��־�ļ�
    ifstream logFile(LOG_FILE.c_str());
    if (!logFile) {
        cerr << "Error opening log file: " << LOG_FILE << endl;
        return ipList; // ���ؿ����� 
    }

    // �ƶ��ļ�ָ�뵽�ļ�ĩβ
    logFile.seekg(-1, ios::end);
    streampos endPos = logFile.tellg(); // ��ʼ������ 

    // ʾ����ȡ��д�����
    unordered_map<string, unsigned int> ipMap; // map��key = IP��ַ��value = vector����ţ�����{key:"192.168.1.1", value:0} 
    
    const int numLines = 1000;
	bool finished = false;
	time_t cutoffTime = 0; // �������ֵ������ = 11927552��Tue May 19 1970 09:12:32 GMT+0800�� 
	while (!finished) {
		auto result = readLinesReverse(logFile, endPos, numLines);
		vector<string> lines = result.first;
		if (lines.size() == 0) break; // ����������ֹ 
    	endPos = result.second; // ÿ���Զ����� 

    	// �����ȡ����
	    for (auto it = lines.rbegin(); it != lines.rend(); ++it) {
	    	string line = *it; // �����õ������Ի�ȡ�ַ���
	    	size_t ipEnd = line.find(' ');
	    	size_t timeStart = line.find('[', ipEnd);
	    	size_t timeEnd = line.find(']', timeStart);
	    	if (ipEnd == string::npos || timeStart == string::npos || timeEnd == string::npos) {
	    		cout << "String FORMAT ERROR: " << line << endl;
				continue; // �쳣����
			}
	    	// ip��ַ 
	    	string ipAddress = line.substr(0, ipEnd);
	    	time_t opTime = parseTimeLogFile(line.substr(timeStart + 1, timeEnd - timeStart - 1));
	    	if (cutoffTime == 0) {
				cutoffTime = opTime - timecount; // ��ʼ������ʱ��
			} else if (opTime < cutoffTime) {
				finished = true;
				break;
			}
			// ����ipList��ipMap
			auto itm = ipMap.find(ipAddress);
		    // ��� ipAddress �Ƿ������ ipMap ��
		    if (itm == ipMap.end()) {
		    	// �������ڣ����������ݣ������ֵ��
		    	ipList.push_back({ipAddress, opTime, 1});
		        ipMap[ipAddress] = ipList.size() - 1;
		    } else {
		    	// �����ڣ������� + 1 
		    	auto& entry = ipList[itm->second];
    			entry.count++; // �޸�count
		    }
	    }
	}
    // �ر��ļ�
    logFile.close();
    return ipList;
}

// �����������쳣���� 
void handleError(const string& errorMessage) {
    cerr << errorMessage << endl;
    exit(1); // �˳����򲢷���1
}

// �ҷ��ʴ�����࣬�ҳ����������������޵�IP��ַ���������idx�� 
vector<IPRecord> findRecExceedLimit(const vector<IPRecord>& ipList, size_t VISIT_TIMES) {
    vector<IPRecord> blackList;

    for (int i = 0; i < ipList.size(); i++) {
    	const auto& record = ipList[i];
        if (record.count > VISIT_TIMES) {
        	blackList.push_back(record);
        }
    }
    return blackList;
}

// ��ȡ�������б�������
vector<IPRecord> getBlackList(const string& BLACKLIST) {
	vector<IPRecord> blackList;
	
    ifstream inFile(BLACKLIST.c_str());
    if (!inFile) return blackList; // �����ݷ��ؿ��б�
    
    vector<string> ipList = readLines(inFile);
    for (string line : ipList) {
    	blackList.push_back(getLineIpRecord(line)); 
	}
    inFile.close();
    return blackList; // ����
}

// ��ȡ�������б�������ipSet 
unordered_set<string> findBlackListIpSet(const string& BLACKLIST) {
	unordered_set<string> ipSet; 
    ifstream inFile(BLACKLIST.c_str());
    if (!inFile) return ipSet; // �����ݷ��ؿ�set 
    
    vector<string> ipList = readLines(inFile);
    for (string line : ipList) {
    	size_t pos = line.find(' '); // ���ҵ�һ���ո��λ��
    	if (pos != string::npos) line = line.substr(0, pos);
    	ipSet.insert(line); 
	}
    inFile.close();
    return ipSet; // ����set 
}

// ���˵��Ѿ������ú������б��еļ�¼
vector<IPRecord> removePermanent(const vector<IPRecord>& ipList, string BLACK_TEMP2) {
    vector<IPRecord> blackList;

    const unordered_set<string> ipSet = findBlackListIpSet(BLACK_TEMP2);
    if (ipSet.empty()) return ipList; // ������ˣ�����ԭʼ���� 
    
    for (int i = 0; i < ipList.size(); i++) {
    	const auto& black = ipList[i];
    	if (ipSet.find(black.ipAddress) == ipSet.end()) {
        	blackList.push_back(black); // ������ 
        }
    }
    return blackList; // �����µĺ��������� 
}

// ����nginx | ���ú����������벻���ڵļ�¼��
void addBlack(vector<IPRecord>& ipList, string BLACKLIST, bool isFull) {
    if (ipList.size() == 0) return; // ����
    const unordered_set<string> ipSet = findBlackListIpSet(BLACKLIST);

    ofstream outFile(BLACKLIST.c_str(), ios::app);
    if (!outFile) handleError("Error opening temporary blacklist file: " + BLACKLIST); // �쳣�˳�
    
    for (IPRecord black : ipList) {
        if (ipSet.find(black.ipAddress) == ipSet.end()) {
            if (isFull) {
                outFile << black.ipAddress << " " << black.opTime << " " << black.count << endl; // �������ú����� 
            } else {
                outFile << black.ipAddress << endl; // ����nginx������ 
            }
        }
    }
    outFile.close();
}

// ������ʱ���������������м�¼��
void addBlackTemp(const vector<IPRecord>& ipList, string BLACKLIST) {
    ofstream outFile(BLACKLIST.c_str(), ios::app);
    if (!outFile) handleError("Error opening temporary blacklist file: " + BLACKLIST); // �쳣�˳�

    for (IPRecord black : ipList) {
    	outFile << black.ipAddress << " " << black.opTime << " " << black.count << endl; // ������ʱ������ 
	}
    outFile.close();
}

// ����ʱ��ת��Ϊ����
size_t dateDiff(time_t opTime) {
    time_t currentTime = time(nullptr); // ��ȡ��ǰʱ���
    size_t secondsInADay = 24 * 60 * 60; // һ�������
    return (currentTime - opTime) / secondsInADay;
}

// ����ʱ��ת��Ϊ����
size_t timeDiff(time_t opTime) {
    time_t currentTime = time(nullptr); // ��ȡ��ǰʱ���
    size_t secondsInADay = 24 * 60 * 60; // һ�������
    return (currentTime - opTime) / secondsInADay;
}

// �Ƴ���ʱ������������������ipList�еļ�¼��
void cutBlack(vector<IPRecord>& ipList, string BLACKLIST) {
    const size_t BLACK_DAYS = 7; // 7���ڼ�¼�ᱣ������ʱ�������������ѯ�ۻ�����
    
    // ipListתΪipSet 
    unordered_set<string> ipSet;
    for (const auto& record : ipList) {
        ipSet.insert(record.ipAddress);
    }

	// ipList����Ϊ��ʱ�������б� 
    ipList = getBlackList(BLACKLIST);

    ofstream outFile(BLACKLIST.c_str(), ios::trunc); // ����ļ����ݲ�����д�� 
    if (!outFile) handleError("Error opening temporary blacklist file: " + BLACKLIST); // �쳣�˳�
    
    for (IPRecord black : ipList) {
	    if (dateDiff(black.opTime) >= BLACK_DAYS) continue; // ���ʱ������7�죬�޳��˼�¼
    	if (ipSet.find(black.ipAddress) != ipSet.end()) continue; // �޳�����ת���ú������ļ�¼
    	outFile << black.ipAddress << " " << black.opTime << " " << black.count << endl; // ������ʱ������ 
	}
    outFile.close();
}

// ����nginx����������д�����ú���������д����ʱ�������� 
void resetBlack(string BLACK_NGINX, string BLACK_TEMP, string BLACK_TEMP2, size_t MIN_STEP) {
	// ��ȡ���ú�����IP��ַ
    unordered_set<string> ipSet = findBlackListIpSet(BLACK_TEMP2);
    // ��ȡ��ʱ������IP��ַ
    vector<IPRecord> blackList = getBlackList(BLACK_TEMP);
    for (IPRecord black: blackList) {
    	if (time(nullptr) - black.opTime > 60 * MIN_STEP) continue; // ����30����ǰ����ʱ��������¼
        ipSet.insert(black.ipAddress); // �ϲ���ʱ������IP��ַ 
	}
	// ��дnginx������
	ofstream outFile(BLACK_NGINX.c_str(), ios::trunc);
    if (!outFile) handleError("Error opening temporary blacklist file: " + BLACK_NGINX); // �쳣�˳�
	// ѭ������ ipSet ��д�뵽�ļ�
    for (const string& ip : ipSet) {
        outFile << ip << endl;
    }
    outFile.close();
}

// �Ƿ���Ϸ��Ӳ�����ĿǰΪ30���ӣ�
bool isMinStep(size_t MIN_STEP) {
	// ��ȡ��ǰʱ���
    auto now = chrono::system_clock::now();
    // ת��Ϊtime_t����
    time_t currentTime = chrono::system_clock::to_time_t(now);
    // ת��Ϊtm�ṹ��
    tm* localTime = localtime(&currentTime);
    // ��ȡ������
    int minutes = localTime->tm_min;
    // �жϷ������Ƿ�Ϊ10�ı���
    return (minutes % MIN_STEP == 0);
}

// ��ȡ�������ú������ļ�¼
vector<IPRecord> getBlackPermenant(string BLACK_TEMP) {
    const size_t BLACK_DAYS = 7; // 7���ڼ�¼�ᱣ������ʱ�������������ѯ�ۻ�����
    const size_t BLACK_TIMES = 6; // 7���ڳ���6�ν���ʱ����������Ϊ�����ú����������÷�IP��
	
    ifstream inFile(BLACK_TEMP.c_str());
    if (!inFile) handleError("Error opening temporary blacklist file: " + BLACK_TEMP); // �쳣�˳�

    inFile.seekg(0);
    map<string, int> ipCountMap; // map��key=������IP��ַ��value=�ļ��г��ֶ��ٴΣ��У�
    string line; // �ļ��ж�ȡһ�в����Դ���
    vector<IPRecord> ipList; // ��������ú�����������IP��ַ�������Ϣ
    while (getline(inFile, line)) {
    	IPRecord record = getLineIpRecord(line);
	    // ʹ��istringstream�������ַ���
	    if (dateDiff(record.opTime) >= BLACK_DAYS) continue; // ���ʱ������7�죬�޳��˼�¼
	    // ������ݵ�map
	    if (ipCountMap.find(record.ipAddress) == ipCountMap.end()) {
            ipCountMap[record.ipAddress] = 1; // ���������ִ��� = 1
        } else {
        	// ���������ִ��� + 1
            if (++ipCountMap[record.ipAddress] == BLACK_TIMES) {
            	ipList.push_back(getLineIpRecord(line)); // �����б� 
			}
        }
    }
    return ipList;
}

/**
 * ��ʱ�������߼���
 * 1��ÿ3���ӣ�180�룩����1500�η��ʼ�¼���ᱻ���뵽��ʱ������BLACK_TEMP [IP ʱ��� ����]
 * 2��7��������6�α�������������ᱻ���뵽���ú�����BLACK_TEMP2 
 * @param file �������ļ� 
 * @param endPos ����ָ��λ�� 
 * @param numLines ��ȡ���� 
 * @return pair<vector<string>, streampos> ��ȡ����б� | ��ָ��λ�� 
 */
int main() {
    const size_t TIME_RANGE = 180; // һ���ж�180���ڵ���־ 
    const size_t VISIT_TIMES = 1500; // ����ÿ3���ӳ���1500�η��ʣ��Żᱻ�Ž�������
    const size_t MIN_STEP = 30; // ÿ��30��������һ�����ú�����
    // ��־�ļ�·��
    string LOG_FILE="/var/log/nginx/wp.edu_access.log"; // ����ϵͳ����Linux��
    #ifdef _WIN32
        LOG_FILE = "./wp.edu_access.log"; // Windowsϵͳ
    #endif
    // �������ļ�·��
    const string BLACK_NGINX="./file/black_nginx.conf"; // nginx������
    const string BLACK_TEMP="./file/black_temp.conf"; // ��ʱ������
    const string BLACK_TEMP2="./file/black_temp_2.conf"; // ��ʱ������-����
    // ��ʱ�ļ�·��
    const string TEMP_FILE="/tmp/blacklist.tmp";

    vector<IPRecord> blackList = getIpList(LOG_FILE, TIME_RANGE); // ��ȡ���1�������з��ʼ�¼
    blackList = findRecExceedLimit(blackList, VISIT_TIMES); // ͳ�ƴ�������������б�
    blackList = removePermanent(blackList, BLACK_TEMP2); // �޳����ú������еļ�¼�����账��
    addBlack(blackList, BLACK_NGINX, false); // ����nginx������
    addBlackTemp(blackList, BLACK_TEMP); // ������ʱ������

    // ÿ��30����ִ��һ�Σ��������ú�������
    if (isMinStep(MIN_STEP)) {
        blackList = getBlackPermenant(BLACK_TEMP); // ��ȡ���ν������ú������ļ�¼
        addBlack(blackList, BLACK_TEMP2, true); // �������ú�����
        cutBlack(blackList, BLACK_TEMP); // �Ƴ���ʱ������
        resetBlack(BLACK_NGINX, BLACK_TEMP, BLACK_TEMP2, MIN_STEP); // ����nginx������
    }
    return 0;
}

