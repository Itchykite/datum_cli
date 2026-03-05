#include <climits>
#include <iostream>
#include <sstream>
#include <curl/curl.h>
#include <cjson/cJSON.h>
#include <string>
#include <cstring>

enum class get_by { ID, NAME, LASTNAME, LOGIN };

size_t WriteCallback(char* data, size_t size, size_t nmemb, void* userp);
CURLcode get_user(std::string user_id, std::ostringstream& response);
CURLcode
create_user(const std::string& imie, const std::string& nazwisko, const std::string& login, const std::string& haslo);
int print_users(get_by by = get_by::ID);
int create_user();
int delete_user(const std::string& user_id);
void print_help();

int main()
{
    curl_global_init(CURL_GLOBAL_DEFAULT);

    std::string options;
    while (true)
    {
        std::cout << "> ";
        std::getline(std::cin, options);

        std::istringstream iss(options);
        std::string cmd;
        iss >> cmd;

        if (cmd == "help")
            print_help();
        else if (cmd == "exit")
            break;
        else if (cmd == "clear")
            system("clear");
        else if (cmd == "print_users")
        {
            get_by by = get_by::ID;

            std::string flag;
            while (iss >> flag)
                if (flag == "--id")
                    by = get_by::ID;
                else if (flag == "--name")
                    by = get_by::NAME;
                else if (flag == "--lastname")
                    by = get_by::LASTNAME;
                else if (flag == "--login")
                    by = get_by::LOGIN;
                else
                    std::cout << "Unknown flag: " << flag << std::endl;

            print_users(by);
        }
        else if (cmd == "create_user")
            create_user();
        else
            continue;
    }

    curl_global_cleanup();

    return 0;
}

size_t WriteCallback(char* data, size_t size, size_t nmemb, void* userp)
{
    *static_cast<std::ostream*>(userp) << data;
    return size * nmemb;
}

CURLcode get_user(std::string user_id, std::ostringstream& response)
{
    CURL* handle = curl_easy_init();
    if (!handle)
        return CURLE_FAILED_INIT;

    std::string url = "http://127.0.0.1:8000/users/" + user_id;

    curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(handle);
    curl_easy_cleanup(handle);
    return res;
}

CURLcode
create_user(const std::string& imie, const std::string& nazwisko, const std::string& login, const std::string& haslo)
{
    CURL* handle = curl_easy_init();
    if (!handle)
        return CURLE_FAILED_INIT;

    std::string url = "http://127.0.0.1:8000/users";

    cJSON* json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "imie", imie.c_str());
    cJSON_AddStringToObject(json, "nazwisko", nazwisko.c_str());
    cJSON_AddStringToObject(json, "login", login.c_str());
    cJSON_AddStringToObject(json, "pass", haslo.c_str());

    char* json_str = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    std::ostringstream response;
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &response);

    curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(handle, CURLOPT_POST, 1L);
    curl_easy_setopt(handle, CURLOPT_POSTFIELDS, json_str);
    curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, strlen(json_str));
    curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(handle);

    curl_slist_free_all(headers);
    curl_easy_cleanup(handle);
    free(json_str);

    std::cout << response.str() << std::endl;

    return res;
}

int print_users(get_by by)
{
    while (true)
    {
        std::string input;
        std::string base_url = "http://127.0.0.1:8000/users";
        std::cout << "Enter ";

        switch (by)
        {
            case get_by::ID:
                std::cout << "user ID: ";
                break;
            case get_by::NAME:
                std::cout << "user first name: ";
                break;
            case get_by::LASTNAME:
                std::cout << "user last name: ";
                break;
            case get_by::LOGIN:
                std::cout << "user login: ";
                break;
        }

        std::getline(std::cin, input);
        if (input == "exit")
            break;

        CURL* handle = curl_easy_init();
        if (!handle)
            return 1;

        std::string url;
        if (by == get_by::ID)
            url = base_url + "/" + input;
        else
        {
            char* esc = curl_easy_escape(handle, input.c_str(), 0);
            if (!esc)
            {
                curl_easy_cleanup(handle);
                return 1;
            }
            if (by == get_by::NAME)
                url = base_url + "?imie=" + esc;
            else if (by == get_by::LASTNAME)
                url = base_url + "?nazwisko=" + esc;
            else if (by == get_by::LOGIN)
                url = base_url + "?login=" + esc;
            curl_free(esc);
        }

        std::ostringstream response;
        curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
        curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(handle, CURLOPT_WRITEDATA, &response);

        CURLcode res = curl_easy_perform(handle);
        curl_easy_cleanup(handle);

        if (res != CURLE_OK)
        {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            continue;
        }

        cJSON* json = cJSON_Parse(response.str().c_str());
        if (!json)
        {
            std::cerr << "Error parsing JSON: " << cJSON_GetErrorPtr() << std::endl;
            continue;
        }

        if (cJSON_IsArray(json))
        {
            int count = cJSON_GetArraySize(json);
            for (int i = 0; i < count; ++i)
            {
                cJSON* user = cJSON_GetArrayItem(json, i);
                cJSON* imie = cJSON_GetObjectItemCaseSensitive(user, "imie");
                cJSON* nazwisko = cJSON_GetObjectItemCaseSensitive(user, "nazwisko");
                cJSON* login = cJSON_GetObjectItemCaseSensitive(user, "login");

                std::cout << " ------------------------------------------- " << std::endl;
                if (cJSON_IsString(imie))
                    std::cout << "| Imie: " << imie->valuestring << std::endl;
                if (cJSON_IsString(nazwisko))
                    std::cout << "| Nazwisko: " << nazwisko->valuestring << std::endl;
                if (cJSON_IsString(login))
                    std::cout << "| Login: " << login->valuestring << std::endl;

                cJSON* accounts = cJSON_GetObjectItemCaseSensitive(user, "accounts");
                if (cJSON_IsArray(accounts))
                {
                    int acc_count = cJSON_GetArraySize(accounts);
                    for (int j = 0; j < acc_count; ++j)
                    {
                        cJSON* account = cJSON_GetArrayItem(accounts, j);
                        cJSON* platform_id = cJSON_GetObjectItemCaseSensitive(account, "platform_id");
                        cJSON* acc_login = cJSON_GetObjectItemCaseSensitive(account, "login");
                        cJSON* haslo = cJSON_GetObjectItemCaseSensitive(account, "haslo");
                        cJSON* opis = cJSON_GetObjectItemCaseSensitive(account, "opis");

                        if (cJSON_IsNumber(platform_id))
                            std::cout << "| Platform ID: " << platform_id->valueint << std::endl;
                        if (cJSON_IsString(acc_login))
                            std::cout << "| Login: " << acc_login->valuestring << std::endl;
                        if (cJSON_IsString(haslo))
                            std::cout << "| Haslo: " << haslo->valuestring << std::endl;
                        if (cJSON_IsString(opis))
                            std::cout << "| Opis: " << opis->valuestring << std::endl;
                        std::cout << " ------------------------------------------- " << std::endl;
                    }
                }
            }
        }
        else if (cJSON_IsObject(json))
        {
            cJSON* imie = cJSON_GetObjectItemCaseSensitive(json, "imie");
            cJSON* nazwisko = cJSON_GetObjectItemCaseSensitive(json, "nazwisko");
            cJSON* login = cJSON_GetObjectItemCaseSensitive(json, "login");
            std::cout << " ------------------------------------------- " << std::endl;
            if (cJSON_IsString(imie))
                std::cout << "| Imie: " << imie->valuestring << std::endl;
            if (cJSON_IsString(nazwisko))
                std::cout << "| Nazwisko: " << nazwisko->valuestring << std::endl;
            if (cJSON_IsString(login))
                std::cout << "| Login: " << login->valuestring << std::endl;

            cJSON* accounts = cJSON_GetObjectItemCaseSensitive(json, "accounts");
            if (cJSON_IsArray(accounts))
            {
                int acc_count = cJSON_GetArraySize(accounts);
                for (int j = 0; j < acc_count; ++j)
                {
                    cJSON* account = cJSON_GetArrayItem(accounts, j);
                    cJSON* platform_id = cJSON_GetObjectItemCaseSensitive(account, "platform_id");
                    cJSON* acc_login = cJSON_GetObjectItemCaseSensitive(account, "login");
                    cJSON* haslo = cJSON_GetObjectItemCaseSensitive(account, "haslo");
                    cJSON* opis = cJSON_GetObjectItemCaseSensitive(account, "opis");

                    if (cJSON_IsNumber(platform_id))
                        std::cout << "| Platform ID: " << platform_id->valueint << std::endl;
                    if (cJSON_IsString(acc_login))
                        std::cout << "| Login: " << acc_login->valuestring << std::endl;
                    if (cJSON_IsString(haslo))
                        std::cout << "| Haslo: " << haslo->valuestring << std::endl;
                    if (cJSON_IsString(opis))
                        std::cout << "| Opis: " << opis->valuestring << std::endl;
                    std::cout << " ------------------------------------------- " << std::endl;
                }
            }
        }

        cJSON_Delete(json);
        std::cout << std::endl;
    }

    return 0;
}

int create_user()
{
    std::string imie, nazwisko, login, haslo;

    std::cout << "Enter first name: ";
    std::getline(std::cin, imie);
    std::cout << "Enter last name: ";
    std::getline(std::cin, nazwisko);
    std::cout << "Enter login: ";
    std::getline(std::cin, login);
    std::cout << "Enter password: ";
    std::getline(std::cin, haslo);

    CURLcode res = create_user(imie, nazwisko, login, haslo);
    if (res != CURLE_OK)
    {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        return 1;
    }

    return 0;
}

int delete_user(const std::string& user_id)
{
    CURL* handle = curl_easy_init();
    if (!handle)
        return CURLE_FAILED_INIT;

    std::string url = "http://127.0.0.1:8000/users/" + user_id;

    curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, "DELETE");

    CURLcode res = curl_easy_perform(handle);
    curl_easy_cleanup(handle);

    return res;
}

void print_help()
{
    std::cout << "Available commands:" << std::endl;
    std::cout << "  help - Show this help message" << std::endl;
    std::cout << "  print_users [--id|--name|--lastname|--login] - Print users filtered by the specified field"
              << std::endl;
    std::cout << "  create_user - Create a new user" << std::endl;
    std::cout << "  delete_user <user_id> - Delete a user by ID" << std::endl;
    std::cout << "  clear - Clear the console" << std::endl;
    std::cout << "  exit - Exit the program" << std::endl;
}
