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
#include <map> // 引入map头文件
#include <utility> // For std::pair
#include <algorithm> // For std::reverse

using namespace std;

// centos7 /etc/nginx目录下，执行：g++ -std=c++11 -o blacklist blacklist.cpp

// 测试工具类：读取char*指定位置的数据 
void readRangeFromBuffer(const char* buffer, size_t start, size_t end, const size_t BUFFER_SIZE = 4096) {
    // 确保起始点和终点不会超出数组边界
    if (start >= BUFFER_SIZE || end >= BUFFER_SIZE || start > end) {
        cerr << "Start or end position is out of bounds or invalid." << endl;
        return;
    }
    size_t length = end - start + 1; // 计算实际读取的长度
    string result(buffer + start, length); // 提取子字符串
    cout << result << endl; // 输出结果
}

/**
 * 将临时结果添加到lines的末尾（用在文件逆序读取） 
 * @param lines 添加列表 
 * @param tempLine 待添加的字符串
 * @param newLine true：新增一行；false：添加到最后一行
 * @return
 */
void getlineReverse(vector<string>& lines, string& tempLine, bool newLine) {
	string str = string(tempLine.rbegin(), tempLine.rend()); // 逆转顺序
	tempLine.clear(); // 清空临时数据 
	if (newLine) {
		lines.push_back(str); // 新增一行 
	} else {
		lines[lines.size()-1] = str + lines[lines.size()-1]; // 添加到最后一行
	}
}

/**
 * 将字符串转为时间戳（nginx默认日志文件）
 * @param timeStr 日期格式，例如：03/Sep/2024:14:05:03 +0800
 * @return time_t 时间戳，从1970年1月1日00时00分00秒（格林威治时间）至现在的总秒数 
 */
time_t parseTimeLogFile(const string& timeStr) {
	// 月份缩写转换为月份索引（0 ~ 11）
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
 * 文件顺序读取：给到文件，顺序读取所有的行
 * @param file 待处理文件
 * @return vector<string> 读取结果列表
 */
vector<string> readLines(ifstream& file) {
    vector<string> lines;

    file.seekg(0);
    string line;

    while (getline(file, line)) {
        lines.push_back(line);
    }
    // 获取当前文件指针位置，即读完指定行后的下一行起始位置
    streampos currentPos = file.tellg();
    return lines;
}

/**
 * 文件顺序读取：给到文件的起始位置，顺序读取指定数量的行
 * @param file 待处理文件 
 * @param startPos 起始指针位置 
 * @param numLines 读取行数 
 * @return pair<vector<string>, streampos> 读取结果列表 | 新指针位置 
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
    // 获取当前文件指针位置，即读完指定行后的下一行起始位置
    streampos currentPos = file.tellg();
    return {lines, currentPos};
}

/**
 * 文件逆序读取：给到文件的结束位置，逆序读取指定数量的行
 * @param file 待处理文件 
 * @param endPos 结束指针位置 
 * @param numLines 读取行数 
 * @return pair<vector<string>, streampos> 读取结果列表 | 新指针位置 
 */
pair<vector<string>, streampos> readLinesReverse(ifstream& file, streampos endPos, size_t numLines) {
    const size_t BUFFER_SIZE = 4096; // 读取的数据块大小
    char bufferArray[BUFFER_SIZE];
    vector<string> lines;
    streampos pos = endPos;
    size_t linesRead = 0; // 已读行数（可能会比linesFinished多1行） 
    size_t linesFinished = 0; // 已写入行数（\n结尾） 

    while (pos > 0 && linesFinished < numLines) {
        size_t bytesToRead = min(static_cast<streampos>(BUFFER_SIZE), pos); // 1 ~ pos的总长度 vs. 4096，两者取小
        pos -= bytesToRead; // pos往前移动待读取字符数量
        file.seekg(pos); // 文件随机读取的初始定位
        file.read(bufferArray, bytesToRead); // 一次性读取

        // 处理读取的数据
        string tempLine; // 临时行内容（逆序写入） 
        for (size_t i = bytesToRead; i > 0; --i) {
            if (bufferArray[i - 1] == '\n' || bufferArray[i - 1] == '\r') {
            	// 遇到windows的\r，跳过处理下一个字符\n（linux为'\n'，windows为'\r\n'，特殊情况如旧版MacOS为'\r'） 
            	if (bufferArray[i - 1] == '\r' && i - 1 > 0 && bufferArray[i - 2] == '\n') continue; // \r\n自动跳过\r 
                if (!tempLine.empty()) {
                    // 反向读取的每行都需要逆序插入
                    bool newLine = linesRead == linesFinished;
                	getlineReverse(lines, tempLine, newLine); 
                    if (newLine) ++linesRead; // 已读取的行数 						
                    ++linesFinished; // 已完成的行数 
                }
                if (linesFinished >= numLines) {
                	pos += (i - 1); // 减去未读数量（下次从此处重新读取） 
                    break; // 一次最多读取多少行（默认 = 1000） 
                }
            } else {
                tempLine += bufferArray[i - 1]; // 字符串组装 
            }
        }
        // 如果最后没有找到换行符，还需要处理剩余的行
        if (linesRead < numLines && !tempLine.empty()) {
            bool newLine = linesRead == linesFinished; // 特殊处理：一行可能远大于4096字节 
        	getlineReverse(lines, tempLine, newLine);
            ++linesRead;
        }
    }
    // 反转结果，以便从文件末尾到文件开头的顺序
    reverse(lines.begin(), lines.end());
    return make_pair(lines, pos);
}

// 定义结构体
struct IPRecord {
    string ipAddress;  // IP 地址
    time_t opTime;  // 时间戳
    unsigned int count; // 计数器
};

/**
 * 把一行string转为IPRecord，时间为时间戳格式 
 */
IPRecord getLineIpRecord(string line) {
    IPRecord record;
    istringstream iss(line);
    // 从输入流中提取IP地址、时间戳和访问次数
    iss >> record.ipAddress >> record.opTime >> record.count;
    return record;
}

// 找最近60秒内访问次数最多，且超过黑名单次数下限的IP地址（返回序号idx） 
vector<IPRecord> getIpList(string LOG_FILE, const int timecount) {
    vector<IPRecord> ipList; // 数组，格式类似{"192.168.1.1", "03/Sep/2024:14:05:16 +0800", 5} 

    // 读取日志文件
    ifstream logFile(LOG_FILE.c_str());
    if (!logFile) {
        cerr << "Error opening log file: " << LOG_FILE << endl;
        return ipList; // 返回空数组 
    }

    // 移动文件指针到文件末尾
    logFile.seekg(-1, ios::end);
    streampos endPos = logFile.tellg(); // 初始化设置 

    // 示例读取和写入操作
    unordered_map<string, unsigned int> ipMap; // map，key = IP地址；value = vector中序号，类似{key:"192.168.1.1", value:0} 
    
    const int numLines = 1000;
	bool finished = false;
	time_t cutoffTime = 0; // 必须设初值，否则 = 11927552（Tue May 19 1970 09:12:32 GMT+0800） 
	while (!finished) {
		auto result = readLinesReverse(logFile, endPos, numLines);
		vector<string> lines = result.first;
		if (lines.size() == 0) break; // 无数据则终止 
    	endPos = result.second; // 每次自动重置 

    	// 输出读取的行
	    for (auto it = lines.rbegin(); it != lines.rend(); ++it) {
	    	string line = *it; // 解引用迭代器以获取字符串
	    	size_t ipEnd = line.find(' ');
	    	size_t timeStart = line.find('[', ipEnd);
	    	size_t timeEnd = line.find(']', timeStart);
	    	if (ipEnd == string::npos || timeStart == string::npos || timeEnd == string::npos) {
	    		cout << "String FORMAT ERROR: " << line << endl;
				continue; // 异常处理
			}
	    	// ip地址 
	    	string ipAddress = line.substr(0, ipEnd);
	    	time_t opTime = parseTimeLogFile(line.substr(timeStart + 1, timeEnd - timeStart - 1));
	    	if (cutoffTime == 0) {
				cutoffTime = opTime - timecount; // 初始化截至时间
			} else if (opTime < cutoffTime) {
				finished = true;
				break;
			}
			// 生成ipList和ipMap
			auto itm = ipMap.find(ipAddress);
		    // 检查 ipAddress 是否存在于 ipMap 中
		    if (itm == ipMap.end()) {
		    	// 键不存在，插入新数据，补充键值对
		    	ipList.push_back({ipAddress, opTime, 1});
		        ipMap[ipAddress] = ipList.size() - 1;
		    } else {
		    	// 键存在，计数器 + 1 
		    	auto& entry = ipList[itm->second];
    			entry.count++; // 修改count
		    }
	    }
	}
    // 关闭文件
    logFile.close();
    return ipList;
}

// 公共函数：异常处理 
void handleError(const string& errorMessage) {
    cerr << errorMessage << endl;
    exit(1); // 退出程序并返回1
}

// 找访问次数最多，且超过黑名单次数下限的IP地址（返回序号idx） 
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

// 获取黑名单列表，并返回
vector<IPRecord> getBlackList(const string& BLACKLIST) {
	vector<IPRecord> blackList;
	
    ifstream inFile(BLACKLIST.c_str());
    if (!inFile) return blackList; // 无数据返回空列表
    
    vector<string> ipList = readLines(inFile);
    for (string line : ipList) {
    	blackList.push_back(getLineIpRecord(line)); 
	}
    inFile.close();
    return blackList; // 返回
}

// 获取黑名单列表，并返回ipSet 
unordered_set<string> findBlackListIpSet(const string& BLACKLIST) {
	unordered_set<string> ipSet; 
    ifstream inFile(BLACKLIST.c_str());
    if (!inFile) return ipSet; // 无数据返回空set 
    
    vector<string> ipList = readLines(inFile);
    for (string line : ipList) {
    	size_t pos = line.find(' '); // 查找第一个空格的位置
    	if (pos != string::npos) line = line.substr(0, pos);
    	ipSet.insert(line); 
	}
    inFile.close();
    return ipSet; // 返回set 
}

// 过滤掉已经在永久黑名单列表中的记录
vector<IPRecord> removePermanent(const vector<IPRecord>& ipList, string BLACK_TEMP2) {
    vector<IPRecord> blackList;

    const unordered_set<string> ipSet = findBlackListIpSet(BLACK_TEMP2);
    if (ipSet.empty()) return ipList; // 无需过滤，返回原始数据 
    
    for (int i = 0; i < ipList.size(); i++) {
    	const auto& black = ipList[i];
    	if (ipSet.find(black.ipAddress) == ipSet.end()) {
        	blackList.push_back(black); // 黑名单 
        }
    }
    return blackList; // 返回新的黑名单数据 
}

// 加入nginx | 永久黑名单（加入不存在的记录）
void addBlack(vector<IPRecord>& ipList, string BLACKLIST, bool isFull) {
    if (ipList.size() == 0) return; // 跳过
    const unordered_set<string> ipSet = findBlackListIpSet(BLACKLIST);

    ofstream outFile(BLACKLIST.c_str(), ios::app);
    if (!outFile) handleError("Error opening temporary blacklist file: " + BLACKLIST); // 异常退出
    
    for (IPRecord black : ipList) {
        if (ipSet.find(black.ipAddress) == ipSet.end()) {
            if (isFull) {
                outFile << black.ipAddress << " " << black.opTime << " " << black.count << endl; // 加入永久黑名单 
            } else {
                outFile << black.ipAddress << endl; // 加入nginx黑名单 
            }
        }
    }
    outFile.close();
}

// 加入临时黑名单（加入所有记录）
void addBlackTemp(const vector<IPRecord>& ipList, string BLACKLIST) {
    ofstream outFile(BLACKLIST.c_str(), ios::app);
    if (!outFile) handleError("Error opening temporary blacklist file: " + BLACKLIST); // 异常退出

    for (IPRecord black : ipList) {
    	outFile << black.ipAddress << " " << black.opTime << " " << black.count << endl; // 加入临时黑名单 
	}
    outFile.close();
}

// 计算时间差并转换为天数
size_t dateDiff(time_t opTime) {
    time_t currentTime = time(nullptr); // 获取当前时间戳
    size_t secondsInADay = 24 * 60 * 60; // 一天的秒数
    return (currentTime - opTime) / secondsInADay;
}

// 计算时间差并转换为天数
size_t timeDiff(time_t opTime) {
    time_t currentTime = time(nullptr); // 获取当前时间戳
    size_t secondsInADay = 24 * 60 * 60; // 一天的秒数
    return (currentTime - opTime) / secondsInADay;
}

// 移除临时黑名单（仅保留不在ipList中的记录）
void cutBlack(vector<IPRecord>& ipList, string BLACKLIST) {
    const size_t BLACK_DAYS = 7; // 7天内记录会保留在临时黑名单，方便查询累积次数
    
    // ipList转为ipSet 
    unordered_set<string> ipSet;
    for (const auto& record : ipList) {
        ipSet.insert(record.ipAddress);
    }

	// ipList设置为临时黑名单列表 
    ipList = getBlackList(BLACKLIST);

    ofstream outFile(BLACKLIST.c_str(), ios::trunc); // 清除文件内容并重新写入 
    if (!outFile) handleError("Error opening temporary blacklist file: " + BLACKLIST); // 异常退出
    
    for (IPRecord black : ipList) {
	    if (dateDiff(black.opTime) >= BLACK_DAYS) continue; // 如果时间差大于7天，剔除此记录
    	if (ipSet.find(black.ipAddress) != ipSet.end()) continue; // 剔除本次转永久黑名单的记录
    	outFile << black.ipAddress << " " << black.opTime << " " << black.count << endl; // 加入临时黑名单 
	}
    outFile.close();
}

// 重置nginx黑名单（先写入永久黑名单，再写入临时黑名单） 
void resetBlack(string BLACK_NGINX, string BLACK_TEMP, string BLACK_TEMP2, size_t MIN_STEP) {
	// 获取永久黑名单IP地址
    unordered_set<string> ipSet = findBlackListIpSet(BLACK_TEMP2);
    // 获取临时黑名单IP地址
    vector<IPRecord> blackList = getBlackList(BLACK_TEMP);
    for (IPRecord black: blackList) {
    	if (time(nullptr) - black.opTime > 60 * MIN_STEP) continue; // 跳过30分钟前的临时黑名单记录
        ipSet.insert(black.ipAddress); // 合并临时黑名单IP地址 
	}
	// 重写nginx黑名单
	ofstream outFile(BLACK_NGINX.c_str(), ios::trunc);
    if (!outFile) handleError("Error opening temporary blacklist file: " + BLACK_NGINX); // 异常退出
	// 循环遍历 ipSet 并写入到文件
    for (const string& ip : ipSet) {
        outFile << ip << endl;
    }
    outFile.close();
}

// 是否符合分钟步长（目前为30分钟）
bool isMinStep(size_t MIN_STEP) {
	// 获取当前时间点
    auto now = chrono::system_clock::now();
    // 转换为time_t类型
    time_t currentTime = chrono::system_clock::to_time_t(now);
    // 转换为tm结构体
    tm* localTime = localtime(&currentTime);
    // 获取分钟数
    int minutes = localTime->tm_min;
    // 判断分钟数是否为10的倍数
    return (minutes % MIN_STEP == 0);
}

// 获取进入永久黑名单的记录
vector<IPRecord> getBlackPermenant(string BLACK_TEMP) {
    const size_t BLACK_DAYS = 7; // 7天内记录会保留在临时黑名单，方便查询累积次数
    const size_t BLACK_TIMES = 6; // 7天内超过6次进临时黑名单，改为进永久黑名单（永久封IP）
	
    ifstream inFile(BLACK_TEMP.c_str());
    if (!inFile) handleError("Error opening temporary blacklist file: " + BLACK_TEMP); // 异常退出

    inFile.seekg(0);
    map<string, int> ipCountMap; // map的key=黑名单IP地址；value=文件中出现多少次（行）
    string line; // 文件中读取一行并予以处理
    vector<IPRecord> ipList; // 需进入永久黑名单的所有IP地址及相关信息
    while (getline(inFile, line)) {
    	IPRecord record = getLineIpRecord(line);
	    // 使用istringstream来解析字符串
	    if (dateDiff(record.opTime) >= BLACK_DAYS) continue; // 如果时间差大于7天，剔除此记录
	    // 添加数据到map
	    if (ipCountMap.find(record.ipAddress) == ipCountMap.end()) {
            ipCountMap[record.ipAddress] = 1; // 黑名单出现次数 = 1
        } else {
        	// 黑名单出现次数 + 1
            if (++ipCountMap[record.ipAddress] == BLACK_TIMES) {
            	ipList.push_back(getLineIpRecord(line)); // 加入列表 
			}
        }
    }
    return ipList;
}

/**
 * 临时黑名单逻辑：
 * 1）每3分钟（180秒）超过1500次访问记录，会被加入到临时黑名单BLACK_TEMP [IP 时间戳 次数]
 * 2）7天内至少6次被列入黑名单，会被加入到永久黑名单BLACK_TEMP2 
 * @param file 待处理文件 
 * @param endPos 结束指针位置 
 * @param numLines 读取行数 
 * @return pair<vector<string>, streampos> 读取结果列表 | 新指针位置 
 */
int main() {
    const size_t TIME_RANGE = 180; // 一次判断180秒内的日志 
    const size_t VISIT_TIMES = 1500; // 必须每3分钟超过1500次访问，才会被放进黑名单
    const size_t MIN_STEP = 30; // 每隔30分钟重算一次永久黑名单
    // 日志文件路径
    string LOG_FILE="/var/log/nginx/wp.edu_access.log"; // 其他系统（如Linux）
    #ifdef _WIN32
        LOG_FILE = "./wp.edu_access.log"; // Windows系统
    #endif
    // 黑名单文件路径
    const string BLACK_NGINX="./file/black_nginx.conf"; // nginx黑名单
    const string BLACK_TEMP="./file/black_temp.conf"; // 临时黑名单
    const string BLACK_TEMP2="./file/black_temp_2.conf"; // 临时黑名单-永久
    // 临时文件路径
    const string TEMP_FILE="/tmp/blacklist.tmp";

    vector<IPRecord> blackList = getIpList(LOG_FILE, TIME_RANGE); // 读取最近1分钟所有访问记录
    blackList = findRecExceedLimit(blackList, VISIT_TIMES); // 统计待进入黑名单的列表
    blackList = removePermanent(blackList, BLACK_TEMP2); // 剔除永久黑名单中的记录（无需处理）
    addBlack(blackList, BLACK_NGINX, false); // 加入nginx黑名单
    addBlackTemp(blackList, BLACK_TEMP); // 加入临时黑名单

    // 每隔30分钟执行一次（加入永久黑名单）
    if (isMinStep(MIN_STEP)) {
        blackList = getBlackPermenant(BLACK_TEMP); // 获取本次进入永久黑名单的记录
        addBlack(blackList, BLACK_TEMP2, true); // 加入永久黑名单
        cutBlack(blackList, BLACK_TEMP); // 移除临时黑名单
        resetBlack(BLACK_NGINX, BLACK_TEMP, BLACK_TEMP2, MIN_STEP); // 重置nginx黑名单
    }
    return 0;
}

