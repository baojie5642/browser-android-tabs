// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/chrome_content_client.h"

#include <string>

#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/test/scoped_command_line.h"
#include "build/build_config.h"
#include "content/public/common/content_switches.h"
#include "extensions/common/constants.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"
#include "url/url_util.h"

namespace {

void CheckUserAgentStringOrdering(bool mobile_device) {
  std::vector<std::string> pieces;

  // Check if the pieces of the user agent string come in the correct order.
  ChromeContentClient content_client;
  std::string buffer = content_client.GetUserAgent();

  pieces = base::SplitStringUsingSubstr(buffer, "Mozilla/5.0 (",
                                        base::TRIM_WHITESPACE,
                                        base::SPLIT_WANT_ALL);
  ASSERT_EQ(2u, pieces.size());
  buffer = pieces[1];
  EXPECT_EQ("", pieces[0]);

  pieces = base::SplitStringUsingSubstr(buffer, ") AppleWebKit/",
                                        base::TRIM_WHITESPACE,
                                        base::SPLIT_WANT_ALL);
  ASSERT_EQ(2u, pieces.size());
  buffer = pieces[1];
  std::string os_str = pieces[0];

  pieces = base::SplitStringUsingSubstr(buffer, " (KHTML, like Gecko) ",
                                        base::TRIM_WHITESPACE,
                                        base::SPLIT_WANT_ALL);
  ASSERT_EQ(2u, pieces.size());
  buffer = pieces[1];
  std::string webkit_version_str = pieces[0];

  pieces = base::SplitStringUsingSubstr(buffer, " Safari/",
                                        base::TRIM_WHITESPACE,
                                        base::SPLIT_WANT_ALL);
  ASSERT_EQ(2u, pieces.size());
  std::string product_str = pieces[0];
  std::string safari_version_str = pieces[1];

  // Not sure what can be done to better check the OS string, since it's highly
  // platform-dependent.
  EXPECT_FALSE(os_str.empty());

  // Check that the version numbers match.
  EXPECT_FALSE(webkit_version_str.empty());
  EXPECT_FALSE(safari_version_str.empty());
  EXPECT_EQ(webkit_version_str, safari_version_str);

  EXPECT_TRUE(
      base::StartsWith(product_str, "Chrome/", base::CompareCase::SENSITIVE));
  if (mobile_device) {
    // "Mobile" gets tacked on to the end for mobile devices, like phones.
    EXPECT_TRUE(
        base::EndsWith(product_str, " Mobile", base::CompareCase::SENSITIVE));
  }
}

}  // namespace


namespace chrome_common {

TEST(ChromeContentClientTest, Basic) {
#if defined(OS_ANDROID)
  const char* const kArguments[] = {"chrome"};
  base::test::ScopedCommandLine scoped_command_line;
  base::CommandLine* command_line = scoped_command_line.GetProcessCommandLine();
  command_line->InitFromArgv(1, kArguments);

  // Do it for regular devices.
  ASSERT_FALSE(command_line->HasSwitch(switches::kUseMobileUserAgent));
  CheckUserAgentStringOrdering(false);

  // Do it for mobile devices.
  command_line->AppendSwitch(switches::kUseMobileUserAgent);
  ASSERT_TRUE(command_line->HasSwitch(switches::kUseMobileUserAgent));
  CheckUserAgentStringOrdering(true);
#else
  CheckUserAgentStringOrdering(false);
#endif
}

#if defined(ENABLE_PLUGINS)
TEST(ChromeContentClientTest, FindMostRecent) {
  std::vector<content::PepperPluginInfo*> version_vector;
  // Test an empty vector.
  EXPECT_FALSE(ChromeContentClient::FindMostRecentPlugin(version_vector));

  // Now test the vector with one element.
  content::PepperPluginInfo info1;
  info1.version = "1.0.0.0";
  version_vector.push_back(&info1);

  content::PepperPluginInfo* most_recent =
      ChromeContentClient::FindMostRecentPlugin(version_vector);
  EXPECT_EQ("1.0.0.0", most_recent->version);

  content::PepperPluginInfo info5;
  info5.version = "5.0.12.1";
  content::PepperPluginInfo info6_12;
  info6_12.version = "6.0.0.12";
  content::PepperPluginInfo info6_13;
  info6_13.version = "6.0.0.13";

  // Test highest version is picked.
  version_vector.clear();
  version_vector.push_back(&info5);
  version_vector.push_back(&info6_12);
  version_vector.push_back(&info6_13);

  most_recent = ChromeContentClient::FindMostRecentPlugin(version_vector);
  EXPECT_EQ("6.0.0.13", most_recent->version);

  // Test that order does not matter, validates tests below.
  version_vector.clear();
  version_vector.push_back(&info6_13);
  version_vector.push_back(&info6_12);
  version_vector.push_back(&info5);

  most_recent = ChromeContentClient::FindMostRecentPlugin(version_vector);
  EXPECT_EQ("6.0.0.13", most_recent->version);

  // Test real scenarios.
  content::PepperPluginInfo component_flash;
  component_flash.version = "4.3.2.1";
  component_flash.is_external = false;
  component_flash.name = "component_flash";

  content::PepperPluginInfo system_flash;
  system_flash.version = "4.3.2.1";
  system_flash.is_external = true;
  system_flash.name = "system_flash";

  // The order here should be:
  // 1. System Flash.
  // 2. Component update.
  version_vector.clear();
  version_vector.push_back(&system_flash);
  version_vector.push_back(&component_flash);
  most_recent = ChromeContentClient::FindMostRecentPlugin(version_vector);
  EXPECT_STREQ("system_flash", most_recent->name.c_str());
}
#endif  // defined(ENABLE_PLUGINS)

TEST(ChromeContentClientTest, AdditionalSchemes) {
  EXPECT_TRUE(url::IsStandard(
      extensions::kExtensionScheme,
      url::Component(0, strlen(extensions::kExtensionScheme))));

  GURL extension_url(
      "chrome-extension://abcdefghijklmnopqrstuvwxyzabcdef/foo.html");
  url::Origin origin(extension_url);
  EXPECT_EQ("chrome-extension://abcdefghijklmnopqrstuvwxyzabcdef",
            origin.Serialize());
}

}  // namespace chrome_common
