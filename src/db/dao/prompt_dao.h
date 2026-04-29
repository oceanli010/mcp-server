#pragma once

#include "../database.h"
#include "../mcp/types.h"
#include <SQLiteCpp/SQLiteCpp.h>

namespace mcp {
    class PromptDao {
    public:
        explicit PromptDao(SQLite::Database& db);

        //创建提示词
        int create(const Prompt& prompt);

        //按id查询
        Prompt getById(int id);

        //按name查询
        Prompt getByName(const std::string& name);

        //获取提示词列表
        std::vector<Prompt> list();

        //更新工具
        void update(int id, const Prompt& prompt);

        //删除工具
        void deleteById(int id);

        //检查存在性
        bool exists(const std::string& name);
        
    private:
        SQLite::Database& db_;
    };

    class PromptMessageDao {
    public:
        explicit PromptMessageDao(SQLite::Database& db);

        //创建消息
        void create(int promptId, const PromptMessage& message, int orderIndex);

        //按id查询
        std::vector<PromptMessage> getByPromptId(int promptId);

        //按id删除
        void deleteByPromptId(int promptId);

    private:
        SQLite::Database& db_;
    };
}
