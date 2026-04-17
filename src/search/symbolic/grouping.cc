#include "grouping.h"
#include "search_algorithms/symbolic_search.h"

namespace grouping {
    std::shared_ptr<GroupingFunction> g_grouping_function_singleton;
    std::shared_ptr<GroupingFunction> g_grouping_function() {
        assert(g_grouping_function_singleton && "no global grouping function set");
        return g_grouping_function_singleton;
    }
    void set_grouping_function(std::shared_ptr<GroupingFunction> grouping_function) {
        assert(!g_grouping_function_singleton && "global grouping function already set");
        g_grouping_function_singleton = grouping_function;
    }

    static class GroupingFunctionCategoryPlugin: public plugins::TypedCategoryPlugin<GroupingFunction> {
    public:
        GroupingFunctionCategoryPlugin(): TypedCategoryPlugin("GroupingFunction") {
            document_synopsis("Grouping Function");
        }
    } _category_plugin;

    class IdGroupingFeature: public plugins::TypedFeature<GroupingFunction, Id> {
    public:
        IdGroupingFeature(): TypedFeature("id") {
            symbolic::SymbolicSearch::add_options_to_feature(*this); //. We only care about the "transform" option
            document_title("Id grouping function");
            document_synopsis("for use in top-k symbolic searches, performs no grouping");
        }

        virtual std::shared_ptr<Id> create_component(const plugins::Options &options) const override {
            auto grouping_func = std::make_shared<Id>(TaskProxy(*options.get<std::shared_ptr<AbstractTask>>("transform")));
            utils::g_log << "Grouping function: " << grouping_func->to_string() << std::endl;
            set_grouping_function(grouping_func);
            return grouping_func;
        }
    };
    static plugins::FeaturePlugin<IdGroupingFeature> _id_plugin;

    class PrefixGroupingFeature: public plugins::TypedFeature<GroupingFunction, Prefix> {
        public:
        PrefixGroupingFeature(): TypedFeature("prefix") {
            symbolic::SymbolicSearch::add_options_to_feature(*this); //. We only care about the "transform" option
            document_title("Prefix grouping function");
            document_synopsis("for use in top-k symbolic searches");
            this->add_option<int>("n", "prefix length", "1");
        }

        virtual std::shared_ptr<Prefix> create_component(const plugins::Options &options) const override {
            auto grouping_func = std::make_shared<Prefix>(TaskProxy(*options.get<std::shared_ptr<AbstractTask>>("transform")), options.get<int>("n"));
            utils::g_log << "Grouping function: " << grouping_func->to_string() << std::endl;
            set_grouping_function(grouping_func);
            return grouping_func;
        }
    };
    static plugins::FeaturePlugin<PrefixGroupingFeature> _prefix_plugin;
}
