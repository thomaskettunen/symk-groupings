#ifndef GROUPING_H
#define GROUPING_H
#include <set>
#include "../task_proxy.h"
#include "../plugins/plugin.h"

namespace grouping {
    using GroupID = int;
    
    class GroupingFunction {
    protected:
        TaskProxy task;
        GroupingFunction(TaskProxy task): task(task) {};
    public:
        virtual GroupID operator()(OperatorID) = 0;
        virtual std::set<GroupID> get_groups() = 0;
        virtual std::string get_group_name(GroupID) = 0;
        virtual std::string to_string() = 0;
        virtual ~GroupingFunction() = default;
    };
    
    class Id : public GroupingFunction {
    public:
        Id(TaskProxy task) : GroupingFunction(task) {}
        GroupID operator()(OperatorID op) override { return op.get_index(); }
        std::set<GroupID> get_groups() override { 
            std::set<GroupID> groups;
            for (size_t id = 0; id < task.get_operators().size(); ++id) {
                groups.insert(id);
            }
            return groups;
        }
        std::string get_group_name(GroupID op) override { return task.get_operators()[op].get_name(); } // For id, GroupID == OperatorID
        std::string to_string() override { return std::string("Grouping(Id)"); }
    };
    
    class IdGroupingFeature: public plugins::TypedFeature<GroupingFunction, Id> {
    public:
        IdGroupingFeature(): TypedFeature("none") {
            document_title("Id grouping function");
            document_synopsis("for use in top-k symbolic searches, performs no grouping");
        }
        
        virtual std::shared_ptr<Id> create_component(const plugins::Options &options) const override {
            auto grouping_func = std::make_shared<Id>(TaskProxy(*options.get<std::shared_ptr<AbstractTask>>("transform")));
            utils::g_log << "Grouping function: " << grouping_func->to_string() << std::endl;
            return grouping_func;
        }
    };
    static plugins::FeaturePlugin<IdGroupingFeature> _id_group_plugin;
    
    class Prefix : public GroupingFunction {
        int n;
        std::unordered_map<std::string, int> group_name_to_group_id;
    public:
        Prefix(TaskProxy task, int n, char s): GroupingFunction(task), n(n), group_name_to_group_id() {}
        
        GroupID operator()(OperatorID op) override {
            int n = this->n;
            std::function<std::string(std::string)> op_name_to_group_name = [n](std::string op_name) {
                size_t pos = 0;
                for (int i = 0; i < n; i++) {
                    pos = op_name.find(' ', pos + (i > 0));
                    if (pos == std::string::npos) {
                        return op_name; //if they are too short whatever we just give back the whole string
                    }
                }
                return (pos != 0) ? op_name.substr(0, pos) : op_name;
                
            };
            
            auto op_name = task.get_operators()[op.get_index()].get_name();
            auto group_name = op_name_to_group_name(op_name);
            GroupID group_id;
            auto it = group_name_to_group_id.find(group_name);
            if (it != group_name_to_group_id.end()) {
                group_id = (group_name_to_group_id)[group_name];
            } else {
                group_id = group_name_to_group_id.size();
                (group_name_to_group_id)[group_name] = group_id;
            }
            return group_id;
        }
        
        std::set<GroupID> get_groups() override { 
            std::set<GroupID> groups;
            for(auto &[name, id] : group_name_to_group_id) {
                groups.insert(id);
            }
            return groups;
        }
        
        std::string get_group_name(GroupID group_no) override {
            for (auto &it : group_name_to_group_id) {
                if (it.second == group_no) return it.first;
            }
            return std::string("No matching group");
        }
        
        std::string to_string() override { return std::string("Grouping(Prefix, " + std::to_string(n) + "')"); };
    };
    
    class PrefixGroupingFeature: public plugins::TypedFeature<GroupingFunction, Prefix> {
        public:
        PrefixGroupingFeature(): TypedFeature("prefix") {
            document_title("Prefix grouping function");
            document_synopsis("for use in top-k symbolic searches");
            this->add_option<int>("n", "prefix length", "1");
        }
        
        virtual std::shared_ptr<Prefix> create_component(const plugins::Options &options) const override {
            auto grouping_func = std::make_shared<Prefix>(TaskProxy(*options.get<std::shared_ptr<AbstractTask>>("transform")), options.get<int>("n"), options.get<char>("s"));
            utils::g_log << "Grouping function: " << grouping_func->to_string() << std::endl;
            return grouping_func;
        }
    };
    static plugins::FeaturePlugin<PrefixGroupingFeature> _prefix_plugin;
}

#endif //  GROUPING_H