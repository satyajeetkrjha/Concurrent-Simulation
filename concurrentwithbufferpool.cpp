#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <ctime>
#include <fstream>
#include <iostream>
#include <mutex>
#include <set>
#include <thread>
#include <vector>
using namespace std;
using namespace chrono;
using namespace literals::chrono_literals;  // s, h, min, ms, us, ns

typedef std::chrono::time_point<std::chrono::system_clock> timePoint;
typedef std::vector<int> state;

const int bufferSize{ 5 };

const int MaxTimePart{ 18000 }, MaxTimeProduct{ 20000 };
const int num_iterations{ 5 }, part_types{ 5 }, product_parts{ 5 };
auto programStartTime = chrono::system_clock::now();

vector<int> loadTime{ 500, 500, 600, 600, 700 };
vector<int> assemblyTime{ 600, 600, 700, 700, 800 };
vector<int> ToFroTime{ 200, 200, 300, 300, 400 };
vector<int> emptyState{ 0, 0, 0, 0, 0 };
std::mutex io_mutex;
std::mutex countermutex;
int counter = 0;
ofstream Out("log.txt");
// OLD CODE STARTS HERE
int target;
vector<vector<int>> res;
vector<vector<int>> res1;
vector<vector<int>> loadOrdersRes;
vector<vector<int>> pickupOrdersRes;
vector<int> probablitity(vector<int> temp) {
    int sz = temp.size();
    set<int> randomset;
    vector<int> v;
    v.resize(5);
    while (temp.size() != 0) {
        int random_variable = std::rand();
        int val = random_variable % 5;
        const bool is_in = randomset.find(val) != randomset.end();
        randomset.insert(val);
        if (is_in == false) {
            v[val] = temp.back();
            temp.pop_back();
        }
    }
    return v;
}

void printPreety(vector<int> v) {
    std::unique_lock<std::mutex> lock(
        io_mutex);
    cout << v[0] << " " << v[1] << " " << v[2] << " " << v[3] << " " << v[4] << endl;
}

void backtrackProducers(vector<vector<int>>& res, vector<int>& temp,
    vector<int>& nums, int start, int remain) {
    if (remain < 0)
        return;
    else if (remain == 0) {
        vector<int> modified = probablitity(temp);
        res.push_back(modified);
    }
    else {
        for (int i = start; i < nums.size(); i++) {
            temp.push_back(nums[i]);
            backtrackProducers(res, temp, nums, i, remain - nums[i]);
            temp.pop_back();
        }
    }
}

void backtrackConsumers(vector<vector<int>>& res, vector<int>& temp,
    vector<int>& nums, int start, int remain) {
    if (remain < 0)
        return;
    else if (remain == 0 && (temp.size() == 3 || temp.size() == 2)) {
        vector<int> modified = probablitity(temp);
        res.push_back(modified);
    }
    else {
        for (int i = start; i < nums.size(); i++) {
            temp.push_back(nums[i]);
            backtrackConsumers(res, temp, nums, i, remain - nums[i]);
            temp.pop_back();
        }
    }
}

vector<int> generateRandomValidLoadOrders(
    const std::vector<vector<int>>& loadOrdersRes,
    const std::vector<int>& loadOrder) {
    // randomly re-generate a new load order, which must re-use the previously
    // un-loaded parts (that got moved back).

    int sz = loadOrdersRes.size();
    int index = 0;
    vector<int> indices;  // contains indices of all valid possible orders which
    // follows above condition
    for (int i = 0; i < sz; i++) {
        vector<int> temp = loadOrdersRes[i];
        if (temp[0] >= loadOrder[0] && temp[1] >= loadOrder[1] &&
            temp[2] >= loadOrder[2] && temp[3] >= loadOrder[3] &&
            temp[4] >= loadOrder[4]) {
            indices.push_back(i);
        }
    }

    int goodsz = indices.size();
    int random_variable = std::rand();
    int val = indices[random_variable % goodsz];
    return loadOrdersRes[val];
}

vector<int> generateRandomValidPickupOrders(
    const std::vector<vector<int>>& pickupOrderRes,
    const std::vector<int>& pickupOrder) {
    int sz = pickupOrderRes.size();
    int index = 0;
    vector<int> indices;
    for (int i = 0; i < sz; i++) {
        vector<int> temp = pickupOrderRes[i];
        if (temp[0] >= pickupOrder[0] && temp[1] >= pickupOrder[1] &&
            temp[2] >= pickupOrder[2] && temp[3] >= pickupOrder[3] &&
            temp[4] >= pickupOrder[4]) {
            indices.push_back(i);
        }
    }
    int goodsz = indices.size();
    int random_variable = std::rand();
    int val = indices[random_variable % goodsz];
    return pickupOrderRes[val];
}
int ProcessTime(const std::vector<int>& loadOrder,
    const std::vector<int>& oldOrder) {
    int time = 0;
    for (int i = 0; i < 5; i++) {
        time += (loadOrder[i] - oldOrder[i]) * loadTime[i];
    }
    return time;
}
int calculateAssemblyTime(const std::vector<int>& cartState,
    const std::vector<int>& assemblyTime) {
    int time = 0;
    for (int i = 0; i < 5; i++) {
        time += (cartState[i] * assemblyTime[i]);
    }
    return time;
}

int MovementTime(const std::vector<int>& LoadOrder,
    const std::vector<int>& ToFroTime) {
    int time = 0;
    for (int i = 0; i < 5; i++) {
        time += (LoadOrder[i] * ToFroTime[i]);
    }
    return time;
}


void printPreety(string data, vector<int> v) {
    Out << data << ": (" << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3]
        << ", " << v[4] << ")" << endl;
}

void logPartWorker(int id, string status, int accumulatedWait, int iteration,
    vector<int> bufferState, vector<int> loadOrder,
    vector<int> updatedBufferState,
    vector<int> updatedLoadOrder,
    std::chrono::time_point<std::chrono::system_clock> t1
) {
    std::unique_lock<std::mutex> lock(io_mutex);

    chrono::microseconds T2(accumulatedWait);
    auto now_ms = std::chrono::duration_cast<std::chrono::microseconds>(
        t1 - programStartTime);
    Out << "Current Time: " << now_ms.count() << " us" << endl;
    Out << "Iteration " << iteration << endl;
    Out << "Part Worker ID: " << id << endl;
    Out << "Status: " << status << endl;
    printPreety("Buffer State", bufferState);
    printPreety("Load Order", loadOrder);
    printPreety("Updated Buffer State", updatedBufferState);
    printPreety("Updated Load Order", updatedLoadOrder);
    Out << "*******************" << endl;
}

void logProductWorker(int id, int iteration, string status, int accumulated,
    vector<int> bufferstate, vector<int> pickuporder,
    vector<int> localstate, vector<int> cartstate,
    vector<int> updatedbufferstate,
    vector<int> updatedpickuporder,
    vector<int> updatedlocalstate1,
    vector<int> updatedcartstate1, int time,
    vector<int> updatedlocalstate2,
    vector<int> updatedcartstate2, int total, bool flag,
    std::chrono::time_point<std::chrono::system_clock> t1,
    std::chrono::time_point<std::chrono::system_clock> t2,
    bool updateflag

) {

    std::unique_lock<std::mutex> lock(io_mutex);

    if (updateflag) {
        counter++;
    }
    auto now_ms = std::chrono::duration_cast<std::chrono::microseconds>(
        t1 - programStartTime);
    auto now_ms1 = std::chrono::duration_cast<std::chrono::microseconds>(
        t2 - programStartTime);

    Out << "Current Time: " << now_ms.count() << " us" << endl;
    Out << "Iteration " << iteration << endl;
    Out << "Product Worker ID: " << id << endl;
    Out << "Status: " << status << endl;
    //Out << "Accumulated Wait Time: " << T2.count() << " us" << endl;
    printPreety("Buffer State", bufferstate);
    printPreety("PickUpOrder", pickuporder);
    printPreety("Local State", localstate);
    printPreety("Cart State", cartstate);
    printPreety("Updated Buffer State", updatedbufferstate);
    printPreety("Updated Pickup Order", updatedpickuporder);
    printPreety("Local state", updatedlocalstate1);
    printPreety("Cart state", updatedcartstate1);

    if (flag) {
        Out << "Current Time: " << now_ms1.count() << " us" << endl;
        printPreety("Updated Local State", updatedlocalstate2);
        printPreety("Updated Cart State", updatedcartstate2);
    }

    Out << "Total Completed Products: " << counter << endl;
    Out << "*******************" << endl;
}

// OLD CODE ENDS HERE
class Buffer {
    std::mutex bufferMutex;
    vector<int> bufferState;
    static const vector<int> bufferCapacity; // static so treated as global variable
    std::condition_variable partCV, productCV;
    bool canLoad(const vector<int>& order);
    bool canPickup(const vector<int>& order);

public:
    struct status {
        std::cv_status timeoutStatus;
        vector <int> bufferbefore, bufferafter;
        timePoint t;
    };
    Buffer() {
        bufferState = emptyState;
    }
    status load(vector<int>& order, const timePoint& until, bool firstTime);
    status pickup(vector<int>& order, vector<int>& cart, vector<int>& local, const timePoint& until, bool firstTime);

} B;

const vector<int> Buffer::bufferCapacity{ 5, 5, 4, 3, 3 };

bool Buffer::canLoad(const state& order) {
    for (int i = 0; i < bufferSize; ++i)
        if (order[i] and (bufferCapacity[i] > bufferState[i])) return true;
    return false;
}

bool Buffer::canPickup(const state& order) {
    for (int i = 0; i < bufferSize; ++i)
        if (order[i] and bufferState[i]) return true;
    return false;
}
Buffer::status Buffer::load(state& LoadOrder, const timePoint& until,
    bool firstTime) {

    Buffer::status status;
    status.timeoutStatus = std::cv_status::no_timeout;
    std::unique_lock<std::mutex> lock(bufferMutex);

    do {
        if (!canLoad(LoadOrder)) {
            if (!firstTime) continue; // wait
            status.bufferbefore = status.bufferafter = bufferState; // cannot load to match output
            status.t = std::chrono::system_clock::now();
            return status;

        }
        // can load now
        status.bufferbefore = bufferState;
        for (int i = 0; i < 5; i++) {
            int diff = bufferCapacity[i] - bufferState[i];
            int possiblechange = min(diff, LoadOrder[i]);
            LoadOrder[i] = LoadOrder[i] - possiblechange;
            bufferState[i] += possiblechange;

        }
        status.bufferafter = bufferState;
        status.t = std::chrono::system_clock::now();
        productCV.notify_all();
        return status;
    } while (productCV.wait_until(lock, until) != std::cv_status::timeout);
    status.bufferbefore = status.bufferafter = bufferState;
    status.timeoutStatus = std::cv_status::timeout;
    status.t = std::chrono::system_clock::now();
    return status;
}


Buffer::status Buffer::pickup(state& pickupOrder, vector<int>& cartState, vector<int>& localState, const timePoint& until,
    bool firstTime) {
    Buffer::status status;
    status.timeoutStatus = std::cv_status::no_timeout;
    
   
    std::unique_lock<std::mutex> lock(bufferMutex);

    do {
        if (!canPickup(pickupOrder)) {
            if (!firstTime) continue;
            status.bufferbefore = status.bufferafter = bufferState;
            status.t = std::chrono::system_clock::now();
            return status;
        }
        // can load
        status.bufferbefore = bufferState;
        for (int i = 0; i < bufferSize; i++) {
            int diff = min(pickupOrder[i], bufferState[i]);
            bufferState[i] -= diff;
            pickupOrder[i] -= diff;
            cartState[i] += diff;
        }
        status.bufferafter = bufferState;
        status.t = std::chrono::system_clock::now();
        partCV.notify_all();
        return status;

    } while (partCV.wait_until(lock, until) != std::cv_status::timeout);

    status.bufferbefore = status.bufferafter = bufferState;
    status.timeoutStatus = std::cv_status::timeout;
    status.t = std::chrono::system_clock::now();
    return status;
}


void PartWorker(int id) {
    vector <int> LoadOrder = emptyState;
    for (int iteration = 0; iteration < num_iterations; iteration++) {
        auto newLoadOrder = generateRandomValidLoadOrders(loadOrdersRes, LoadOrder);
        int processTime = ProcessTime(newLoadOrder, LoadOrder);  // t1 but there is no previous order in this situation
        LoadOrder = newLoadOrder;
        int movementTime = MovementTime(LoadOrder, ToFroTime);  // t2 as per state diagram
        auto T = movementTime + processTime;
        std::this_thread::sleep_for(std::chrono::microseconds(T));
        auto wait_end_time = std::chrono::system_clock::now() +
            std::chrono::microseconds(MaxTimePart);
        bool firsttime = true;
        while (true) {
            // values are passed by reference here
            auto prevLoadorder = LoadOrder;
            auto status = B.load(LoadOrder, wait_end_time, firsttime);
            string logtype;

            logtype = firsttime == true ? "New Load order" : "Wakeup -Notified";
            if (LoadOrder != emptyState && status.timeoutStatus == std::cv_status::timeout) {
                logtype = "Wakeup-Timeout";
            }


            logPartWorker(id, logtype, 100, iteration, status.bufferbefore, prevLoadorder, status.bufferafter, LoadOrder, status.t);


            // we will log here with values passed and values coming to us from the function
            firsttime = false;
            if (LoadOrder == emptyState || status.timeoutStatus == std::cv_status::timeout) {
                break;
            }
        }
        movementTime = MovementTime(LoadOrder, ToFroTime);
        chrono::microseconds T1(movementTime);
        std::this_thread::sleep_for(T1);  // THREAD SLEEPNIG HERE

    }
}

void ProductWorker(int id) {
    auto pickupOrder = emptyState;
    auto cartState = emptyState;
    auto localState = emptyState;
    for (int i = 1; i <= num_iterations; i++) {
        pickupOrder = generateRandomValidPickupOrders(pickupOrdersRes, localState);

        auto calT1 = std::chrono::system_clock::now();
        auto calT2 = std::chrono::system_clock::now();
        auto waitTimeDefined = std::chrono::system_clock::now() +
            std::chrono::microseconds(MaxTimePart);
        bool firsttime = true;
        // this line as where it should be is big confusion as of now

        for (int i = 0; i < 5; i++) {
            pickupOrder[i] = pickupOrder[i] - localState[i];
        }
        while (true) {

            auto oldPickUpOrder = pickupOrder;
            auto oldCartState = cartState;
            auto oldLocalState = localState;
            auto status = B.pickup(pickupOrder, cartState, localState, waitTimeDefined, firsttime);
            string logtype;

            logtype = firsttime == true ? "New Pickup order" : "Wakeup - Notified";

            if (pickupOrder != emptyState && status.timeoutStatus == std::cv_status::timeout) {
                logtype = "Wakeup-Timeout";
            }
            bool flag = false;
            if (pickupOrder == emptyState) {
                flag = true;
            }
            logProductWorker(i, id, logtype, 10, status.bufferbefore,
                oldPickUpOrder, oldLocalState, oldCartState,
                status.bufferafter, pickupOrder, localState, cartState, 100,
                localState, cartState, counter, flag, status.t, status.t, false);

            firsttime = false;


            // pickup order all taken case and timeout case both handleded here
            if (pickupOrder == emptyState || status.timeoutStatus == std::cv_status::timeout) break;
        }
        int movementTime = MovementTime(cartState, ToFroTime);
        chrono::microseconds processAndMovement(movementTime);
        std::this_thread::sleep_for(processAndMovement);

        for (int i = 0; i < 5; i++) {
            localState[i] += cartState[i];
        }
        cartState = emptyState;
        if (pickupOrder == emptyState) {

            std::unique_lock<std::mutex> lock(countermutex);
            counter++;
            auto assemblyTimecalculated =
                calculateAssemblyTime(localState, assemblyTime);
            chrono::microseconds T1(assemblyTimecalculated);
            std::this_thread::sleep_for(T1);  // THREAD SLEEPNIG HERE
            cartState = localState = emptyState;
        }
    }
}


int main() {
    const int m = 2, n = 2;
    // m: number of Part Workers and n is  number of Product Workers
    int target = 5;
    vector<int> nums, temp;
    nums = { 1, 2, 3, 4, 5 };

    // backtrack for Conusmers and producers begins and don't touch this at all
    backtrackProducers(res, temp, nums, 0, target);

    for (int i = 0; i < res.size(); i++) {
        vector<int> vec = res[i];
        // generate it's next permutation and push it
        sort(vec.begin(), vec.end());
        do {
            loadOrdersRes.push_back(vec);
        } while (std::next_permutation(vec.begin(), vec.end()));
    }

    temp.clear();
    backtrackConsumers(res1, temp, nums, 0, target);
    for (int i = 0; i < res1.size(); i++) {
        vector<int> vec = res1[i];
        // generate it's next permutation and push it
        sort(vec.begin(), vec.end());
        do {
            pickupOrdersRes.push_back(vec);
        } while (std::next_permutation(vec.begin(), vec.end()));
    }

    cout << loadOrdersRes.size() << endl;
    cout << pickupOrdersRes.size() << endl;



    vector<thread> PartW, ProductW;
    for (int i = 0; i < m; ++i) {
        PartW.emplace_back(PartWorker, i + 1);
    }
    for (int i = 0; i < n; ++i) {
        ProductW.emplace_back(ProductWorker, i + 1);
    }
    for (auto& i : PartW) i.join();
    for (auto& i : ProductW) i.join();
    cout << "Finish!" << endl;
    return 0;
}
