#include "TestFrameworks.h"

#include "Json.h"
#include "RestApi.h"
#include "Storage.h"

using namespace fix;
using namespace std::string_literals;

class StorageMock : public Storage {
public:
  MAKE_CONST_MOCK0(selectMaxIssueID, unsigned(), override);
  MAKE_MOCK1(insertIssueIncreasedID, Json(Json const& requestedIssue), override);
};

TEST_CASE( "Creating an issue...", "[issue]" ) {
  StorageMock storage;
  RestApi api{storage};

  std::string uri = "/issue/new";
  std::string method = "POST";
  auto requestedIssue = Json {
      { "data", {
        { "summary", "New Issue" },
        { "description", "Some lengthy text here to describe the issue" }
      }}
  };
  auto request = requestedIssue.dump();


  SECTION("... inserts the issue into storage and returns it with its data if the input is correct.") {
    auto expectedIssue = requestedIssue;

    expectedIssue["data"]["ID"] = 1;
    REQUIRE_CALL(storage, insertIssueIncreasedID(requestedIssue)).RETURN(expectedIssue);
    auto response = api.process(uri, method, request);

    REQUIRE(response.content == expectedIssue);
    REQUIRE(response.httpCode == 200);
  }


  SECTION("... returns status 400 with corresponding error message if...") {
    auto status400Json = Json {{ "status", 400 }};
    RestApi::Response response = {Json{}, 0};

    SECTION("... the method is wrong.") {
      status400Json["error"] = "expected POST method for " + uri;
      response = api.process(uri, "GET", request);
    }

    SECTION("... the request can not be parsed.") {
      status400Json["error"] = "error parsing request";
      response = api.process(uri, method, "{ some non-json string");
    }

    SECTION("... the request contains no issue data.") {
      status400Json["error"] = "request contains no data";
      response = api.process(uri, method, "{}");
    }

    SECTION("... the request contains an ID for the new issue.") {
      status400Json["error"] = "can not create issue with predefined ID";
      auto requestedIssueWithID = requestedIssue;
      requestedIssueWithID["data"]["ID"] = 44;
      response = api.process(uri, method, requestedIssueWithID.dump());
    }

    for (auto name : {"summary"s, "description"s}) {
      SECTION("... the requested issue does not contain the "s + name + " attribute"s) {
        status400Json["error"] = "issue is missing required attribute " + name;
        auto requestedIssueWithoutAttribute = requestedIssue;
        requestedIssueWithoutAttribute["data"].erase(name);
        response = api.process(uri, method, requestedIssueWithoutAttribute.dump());
      }
    }

    CHECK(response.content == status400Json);
    CHECK(response.httpCode == 400);
  }
}
