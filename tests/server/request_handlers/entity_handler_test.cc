#include "entity_handler.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "handler_registry.h"
#include "mock_filesystem.h"

using ::testing::Return;

class EntityHandlerTest : public ::testing::Test {
   protected:
    std::unique_ptr<MockFilesystem> mock_fs;
    std::unique_ptr<EntityHandler> handler;
    MockFilesystem* mock_fs_ptr;

    void SetUp() override {
        std::map<std::string, std::string> args = {
            {"root", "/tmp/mock_fs"}  // writable safe path
        };

        handler = std::make_unique<EntityHandler>("/api/entity", args);

        auto mock = std::make_unique<MockFilesystem>();
        mock_fs_ptr = mock.get();
        handler->set_filesystem(std::move(mock));
    }

    MockFilesystem* fs() {
        return mock_fs_ptr;
    }
};

TEST_F(EntityHandlerTest, GetWithValidIdReturnsData) {
    HttpRequest request;
    request.method = "GET";
    request.path = "/api/entity/1";

    fs()->write("entity", "1", "{\"name\":\"Whiskers\"}");

    auto response = handler->handle_request(request);
    EXPECT_EQ(response->status, StatusCode::OK);
    EXPECT_EQ(response->headers["Content-Type"], "application/json");
    EXPECT_EQ(response->body, "{\"name\":\"Whiskers\"}");
}

TEST_F(EntityHandlerTest, GetWithoutIdReturnsList) {
    fs()->write("entity", "1", "alpha");
    fs()->write("entity", "2", "beta");

    HttpRequest request;
    request.method = "GET";
    request.path = "/api/entity";

    auto response = handler->handle_request(request);
    EXPECT_EQ(response->status, StatusCode::OK);
    EXPECT_EQ(response->headers["Content-Type"], "application/json");
    EXPECT_EQ(response->body, "[\"1\",\"2\"]");
}

TEST_F(EntityHandlerTest, PostCreatesNewEntity) {
    HttpRequest request;
    request.method = "POST";
    request.path = "/api/entity";
    request.body = "{\"msg\":\"hello\"}";

    auto response = handler->handle_request(request);
    EXPECT_EQ(response->status, StatusCode::OK);
    EXPECT_EQ(response->headers["Content-Type"], "application/json");
    EXPECT_TRUE(response->body.find("\"id\":") != std::string::npos);
}

TEST_F(EntityHandlerTest, PutWithIdUpdatesEntity) {
    fs()->write("entity", "1", "old value");

    HttpRequest request;
    request.method = "PUT";
    request.path = "/api/entity/1";
    request.body = "updated data";

    auto response = handler->handle_request(request);
    EXPECT_EQ(response->status, StatusCode::OK);
    EXPECT_EQ(fs()->read("entity", "1"), "updated data");
}

TEST_F(EntityHandlerTest, DeleteWithValidIdRemovesEntity) {
    fs()->write("entity", "1", "to delete");

    HttpRequest request;
    request.method = "DELETE";
    request.path = "/api/entity/1";

    auto response = handler->handle_request(request);
    EXPECT_EQ(response->status, StatusCode::OK);
    EXPECT_FALSE(fs()->exists("entity", "1"));
}

TEST_F(EntityHandlerTest, GetWithInvalidIdReturnsNotFound) {
    HttpRequest request;
    request.method = "GET";
    request.path = "/api/entity/999";

    auto response = handler->handle_request(request);
    EXPECT_EQ(response->status, StatusCode::NOT_FOUND);
    EXPECT_TRUE(response->body.find("No such entity") != std::string::npos);
}

TEST_F(EntityHandlerTest, DeleteWithInvalidIdReturnsNotFound) {
    HttpRequest request;
    request.method = "DELETE";
    request.path = "/api/entity/999";

    auto response = handler->handle_request(request);
    EXPECT_EQ(response->status, StatusCode::NOT_FOUND);
}

TEST_F(EntityHandlerTest, PutWithoutIdReturnsBadRequest) {
    HttpRequest request;
    request.method = "PUT";
    request.path = "/api/entity";
    request.body = "should fail";

    auto response = handler->handle_request(request);
    EXPECT_EQ(response->status, StatusCode::BAD_REQUEST);
}

TEST_F(EntityHandlerTest, DeleteWithoutIdReturnsBadRequest) {
    HttpRequest request;
    request.method = "DELETE";
    request.path = "/api/entity";

    auto response = handler->handle_request(request);
    EXPECT_EQ(response->status, StatusCode::BAD_REQUEST);
}

TEST_F(EntityHandlerTest, UnsupportedMethodReturnsBadRequest) {
    HttpRequest request;
    request.method = "PATCH";
    request.path = "/api/entity/1";

    auto response = handler->handle_request(request);
    EXPECT_EQ(response->status, StatusCode::BAD_REQUEST);
    EXPECT_EQ(response->body, "Unsupported HTTP method.");
}

TEST_F(EntityHandlerTest, ThrowsWhenRootArgumentMissing) {
    std::map<std::string, std::string> empty_args;
    EXPECT_THROW({ EntityHandler handler("/api/entity", empty_args); }, std::runtime_error);
}

TEST_F(EntityHandlerTest, GetFilesystemReturnsSetFilesystem) {
    EXPECT_EQ(handler->get_filesystem(), mock_fs_ptr);
}

TEST_F(EntityHandlerTest, PutWithEmptyBody) {
    fs()->write("entity", "1", "old value");

    HttpRequest request;
    request.method = "PUT";
    request.path = "/api/entity/1";
    request.body = "";

    auto response = handler->handle_request(request);
    EXPECT_EQ(response->status, StatusCode::OK);
    EXPECT_EQ(fs()->read("entity", "1"), "");
}

TEST_F(EntityHandlerTest, PostWithEmptyBody) {
    HttpRequest request;
    request.method = "POST";
    request.path = "/api/entity";
    request.body = "";

    auto response = handler->handle_request(request);
    EXPECT_EQ(response->status, StatusCode::OK);
}