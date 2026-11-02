#pragma once

#include <QString>

class ChatSession;

// 세션 전체를 마크다운 파일로 내보낸다.
namespace ChatExporter {
bool exportMarkdown(const ChatSession& session, const QString& userName, const QString& filePath, QString* error = nullptr);
QString suggestedFileName(const ChatSession& session);
}
