#include "gtest/gtest.h"

#include "porytiles/utilities/text/plain_text_formatter.hpp"
#include "porytiles/xcut/diagnostics/buffered_user_diagnostics.hpp"
#include "porytiles/xcut/diagnostics/diagnostic_tag_filter.hpp"
#include "porytiles/xcut/diagnostics/filtered_user_diagnostics.hpp"

using namespace porytiles;

TEST(DiagnosticTagFilterTests, EmptyFilterHidesAllTags)
{
    DiagnosticTagFilter filter{{}, {}};

    EXPECT_FALSE(filter.should_show("tile-sharing-result-summary"));
    EXPECT_FALSE(filter.should_show("nothing-to-do"));
}

TEST(DiagnosticTagFilterTests, IncludeShowsMatchingTag)
{
    DiagnosticTagFilter filter{{}, {"nothing-to-do"}};

    EXPECT_TRUE(filter.should_show("nothing-to-do"));
    EXPECT_FALSE(filter.should_show("tile-sharing-result-summary"));
}

TEST(DiagnosticTagFilterTests, WildcardIncludeShowsAllTags)
{
    DiagnosticTagFilter filter{{}, {".*"}};

    EXPECT_TRUE(filter.should_show("nothing-to-do"));
    EXPECT_TRUE(filter.should_show("tile-sharing-result-summary"));
}

TEST(DiagnosticTagFilterTests, ExcludeOverridesInclude)
{
    DiagnosticTagFilter filter{{"tile-sharing-.*"}, {".*"}};

    EXPECT_FALSE(filter.should_show("tile-sharing-result-summary"));
    EXPECT_TRUE(filter.should_show("nothing-to-do"));
}

TEST(DiagnosticTagFilterTests, ExcludeWithoutIncludeHidesAllTags)
{
    DiagnosticTagFilter filter{{"tile-sharing-.*"}, {}};

    EXPECT_FALSE(filter.should_show("tile-sharing-result-summary"));
    EXPECT_FALSE(filter.should_show("nothing-to-do"));
}

TEST(DiagnosticTagFilterTests, PatternsUseRegexSearchSemantics)
{
    // Patterns are matched with regex_search, so an unanchored pattern matches anywhere in the tag
    DiagnosticTagFilter filter{{}, {"sharing"}};

    EXPECT_TRUE(filter.should_show("tile-sharing-result-summary"));
    EXPECT_FALSE(filter.should_show("nothing-to-do"));
}

TEST(DiagnosticTagFilterTests, MultipleIncludePatterns)
{
    DiagnosticTagFilter filter{{}, {"nothing-to-do", "dual-layer-drop"}};

    EXPECT_TRUE(filter.should_show("nothing-to-do"));
    EXPECT_TRUE(filter.should_show("dual-layer-drop"));
    EXPECT_FALSE(filter.should_show("tile-sharing-result-summary"));
}

TEST(FilteredUserDiagnosticsTests, RemarksAndWarningsFilteredIndependently)
{
    PlainTextFormatter formatter{};
    BufferedUserDiagnostics buffered{};
    FilteredUserDiagnostics filtered{
        &formatter, &buffered, DiagnosticTagFilter{{}, {"shown-warning"}}, DiagnosticTagFilter{{}, {"shown-remark"}}};

    filtered.warning("shown-warning", std::vector<std::string>{"warning text"});
    filtered.warning("hidden-warning", std::vector<std::string>{"warning text"});
    filtered.remark("shown-remark", std::vector<std::string>{"remark text"});
    filtered.remark("hidden-remark", std::vector<std::string>{"remark text"});

    EXPECT_EQ(buffered.warnings().size(), 1);
    EXPECT_EQ(buffered.remarks().size(), 1);
}

TEST(FilteredUserDiagnosticsTests, DefaultFiltersHideRemarksAndWarnings)
{
    PlainTextFormatter formatter{};
    BufferedUserDiagnostics buffered{};
    FilteredUserDiagnostics filtered{&formatter, &buffered, DiagnosticTagFilter{{}, {}}, DiagnosticTagFilter{{}, {}}};

    filtered.warning("any-warning", std::vector<std::string>{"warning text"});
    filtered.remark("any-remark", std::vector<std::string>{"remark text"});

    EXPECT_TRUE(buffered.warnings().empty());
    EXPECT_TRUE(buffered.remarks().empty());
}

TEST(FilteredUserDiagnosticsTests, ErrorsBypassFilters)
{
    PlainTextFormatter formatter{};
    BufferedUserDiagnostics buffered{};
    FilteredUserDiagnostics filtered{&formatter, &buffered, DiagnosticTagFilter{{}, {}}, DiagnosticTagFilter{{}, {}}};

    filtered.error("any-error", std::vector<std::string>{"error text"});
    filtered.error_note("any-error", std::vector<std::string>{"note text"});

    EXPECT_EQ(buffered.errors().size(), 1);
    EXPECT_EQ(buffered.error_notes().size(), 1);
}

TEST(FilteredUserDiagnosticsTests, NotesFollowParentTagFilter)
{
    PlainTextFormatter formatter{};
    BufferedUserDiagnostics buffered{};
    FilteredUserDiagnostics filtered{
        &formatter, &buffered, DiagnosticTagFilter{{}, {"shown-warning"}}, DiagnosticTagFilter{{}, {"shown-remark"}}};

    filtered.warning_note("shown-warning", std::vector<std::string>{"note text"});
    filtered.warning_note("hidden-warning", std::vector<std::string>{"note text"});
    filtered.remark_note("shown-remark", std::vector<std::string>{"note text"});
    filtered.remark_note("hidden-remark", std::vector<std::string>{"note text"});

    EXPECT_EQ(buffered.warning_notes().size(), 1);
    EXPECT_EQ(buffered.remark_notes().size(), 1);
}
