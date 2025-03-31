# blacklist.cpp 提纲文档

## 1. 程序概述
- **功能**：Nginx服务器黑名单管理工具
- **运行环境**：CentOS 7，需要在/etc/nginx目录下执行
- **编译命令**：`g++ -std=c++11 -o blacklist blacklist.cpp`
- **主要目的**：自动检测并阻止短时间内频繁访问服务器的IP地址

## 2. 主要功能
- 分析Nginx访问日志，识别短时间内访问频率过高的IP地址
- 维护临时黑名单和永久黑名单系统
- 自动更新Nginx配置文件，阻止黑名单IP的访问
- 定期清理过期的黑名单记录
- 支持Windows和Linux系统路径配置

## 3. 数据结构
- **IPRecord结构体**：存储IP地址、操作时间和访问次数
- **容器使用**：
  - `vector<string>`：存储读取的日志行
  - `vector<IPRecord>`：存储IP记录列表
  - `unordered_map<string, unsigned int>`：快速查找IP地址对应的索引
  - `unordered_map<string, int>`：存储月份名称到数字的映射
  - `unordered_set<string>`：存储唯一的IP地址集合
  - `map<string, int>`：统计IP出现次数

## 4. 关键函数和方法

### 4.1 文件读取函数
- `readLines(ifstream& file)`：顺序读取整个文件
- `readLines(ifstream& file, streampos startPos, size_t numLines)`：从指定位置顺序读取指定行数
- `readLinesReverse(ifstream& file, streampos endPos, size_t numLines)`：从文件末尾反向读取指定行数
- `readRangeFromBuffer(const char* buffer, size_t start, size_t end)`：读取缓冲区指定范围的内容

### 4.2 时间处理函数
- `parseTimeLogFile(const string& timeStr)`：将Nginx日志时间格式转换为time_t
- `dateDiff(time_t opTime)`：计算给定时间与当前时间的天数差
- `timeDiff(time_t opTime)`：计算时间差（与dateDiff功能相同）
- `getCurrentTime()`：获取当前时间
- `printCurrentTime()`：格式化输出当前时间
- `isMinStep(size_t MIN_STEP)`：判断当前分钟是否为指定步长的倍数

### 4.3 IP记录处理函数
- `getIpList(string LOG_FILE, const int timecount)`：获取指定时间范围内的IP访问记录
- `findRecExceedLimit(const vector<IPRecord>& ipList, size_t VISIT_TIMES)`：查找访问次数超过限制的IP
- `getLineIpRecord(string line)`：将字符串转换为IPRecord结构体
- `getBlackList(const string& BLACKLIST)`：读取黑名单文件内容
- `findBlackListIpSet(const string& BLACKLIST)`：获取黑名单IP地址集合

### 4.4 黑名单管理函数
- `addBlack(vector<IPRecord>& ipList, string BLACKLIST, bool isFull)`：添加IP到黑名单
- `addBlackTemp(const vector<IPRecord>& ipList, string BLACKLIST)`：添加IP到临时黑名单
- `cutBlack(vector<IPRecord>& ipList, string BLACKLIST)`：移除过期的临时黑名单记录
- `resetBlack(string BLACK_NGINX, string BLACK_TEMP, string BLACK_TEMP2, size_t MIN_STEP)`：重置Nginx黑名单配置
- `removePermanent(const vector<IPRecord>& ipList, string BLACK_TEMP2)`：过滤掉已在永久黑名单中的记录
- `getBlackPermenant(string BLACK_TEMP)`：获取需要添加到永久黑名单的记录

### 4.5 辅助函数
- `handleError(const string& errorMessage)`：处理错误并退出程序
- `getlineReverse(vector<string>& lines, string& tempLine, bool newLine)`：反向读取时添加行到列表

## 5. 工作流程
1. **初始化**：设置时间范围、访问次数阈值、更新间隔等参数
2. **读取日志**：从Nginx访问日志中读取最近一段时间内的记录
3. **统计分析**：
   - 统计每个IP的访问次数
   - 筛选出访问次数超过阈值的IP
   - 过滤掉已在永久黑名单中的IP
4. **更新黑名单**：
   - 将筛选出的IP添加到Nginx配置文件和临时黑名单
   - 定期（默认30分钟）检查临时黑名单，将多次被列入的IP添加到永久黑名单
   - 清理过期的临时黑名单记录
   - 重新生成Nginx配置文件
5. **完成循环**：程序执行完成，返回状态码

## 6. 配置参数
- **TIME_RANGE**：检查的时间范围（360秒，即6分钟）
- **VISIT_TIMES**：访问次数阈值（2600次/6分钟）
- **MIN_STEP**：更新永久黑名单的时间间隔（30分钟）
- **BLACK_DAYS**：临时黑名单记录保留天数（7天）
- **BLACK_TIMES**：IP被列入永久黑名单的条件（7天内被列入临时黑名单6次）

## 7. 文件说明
- **LOG_FILE**：Nginx访问日志文件路径
  - Linux: `/var/log/nginx/wp.edu_access.log`
  - Windows: `./file/wp.edu_access.log`
- **BLACK_NGINX**：Nginx黑名单配置文件路径
  - Linux: `/etc/nginx/file/black_nginx.conf`
  - Windows: `./file/black_nginx.conf`
- **BLACK_TEMP**：临时黑名单文件路径
  - Linux: `/etc/nginx/file/black_temp.conf`
  - Windows: `./file/black_temp.conf`
- **BLACK_TEMP2**：永久黑名单文件路径
  - Linux: `/etc/nginx/file/black_temp_2.conf`
  - Windows: `./file/black_temp_2.conf`

## 8. 黑名单机制说明
- **临时黑名单**：记录格式为 `[IP地址] [时间戳] [访问次数]`
  - 每6分钟检查一次，将6分钟内访问超过2600次的IP添加到临时黑名单
  - 记录保留7天后自动清理
- **永久黑名单**：记录格式为 `[IP地址] [时间戳] [访问次数]`
  - 每30分钟检查一次临时黑名单
  - 7天内被列入临时黑名单6次的IP会被添加到永久黑名单
- **Nginx配置**：记录格式为 `[IP地址] 1;`
  - 包含所有临时和永久黑名单中的IP
  - 每次程序运行时更新
  - 每30分钟完全重置一次
