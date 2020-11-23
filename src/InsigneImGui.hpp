namespace stone
{
// -----------------------------------------------------------------------------

template <class TAllocator>
DebugLogWindow<TAllocator>::DebugLogWindow(TAllocator* allocator)
	: Buffer(allocator)
	, LineOffsets(allocator)
{
	AutoScroll = true;
	Clear();
}

template <class TAllocator>
void DebugLogWindow<TAllocator>::Clear()
{
	Buffer.clear();
	LineOffsets.clear();
	LineOffsets.push_back(0);
}

template <class TAllocator>
void DebugLogWindow<TAllocator>::AddLog(const_cstr logStr)
{
	int oldSize = Buffer.get_size();
	Buffer.append(logStr);
	Buffer.append("\n");
	for (int newSize = Buffer.get_size(); oldSize < newSize; oldSize++)
	{
		if (Buffer[oldSize] == '\n')
		{
			LineOffsets.push_back(oldSize + 1);
		}
	}
}

template <class TAllocator>
void DebugLogWindow<TAllocator>::Draw(const_cstr title)
{
	ImGui::Begin(title);

	// Options menu
	if (ImGui::BeginPopup("Options"))
	{
		ImGui::Checkbox("Auto-scroll", &AutoScroll);
		ImGui::EndPopup();
	}

	// Main window
	if (ImGui::Button("Options"))
	{
		ImGui::OpenPopup("Options");
	}
	ImGui::SameLine();
	bool clear = ImGui::Button("Clear");
	ImGui::SameLine();
	bool copy = ImGui::Button("Copy");
	ImGui::SameLine();

	ImGui::Separator();
	ImGui::BeginChild("scrolling", ImVec2(0,0), false, ImGuiWindowFlags_HorizontalScrollbar);

	if (clear)
	{
		Clear();
	}
	if (copy)
	{
		ImGui::LogToClipboard();
	}

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
	const c8* buf = Buffer.begin();
	const c8* bufEnd = Buffer.end();

	ImGuiListClipper clipper;
	clipper.Begin(LineOffsets.get_size());
	while (clipper.Step())
	{
		for (int lineNo = clipper.DisplayStart; lineNo < clipper.DisplayEnd; lineNo++)
		{
			const c8* lineStart = buf + LineOffsets[lineNo];
			const c8* lineEnd = (lineNo + 1 < LineOffsets.get_size()) ? (buf + LineOffsets[lineNo + 1] - 1) : bufEnd;
			ImGui::TextUnformatted(lineStart, lineEnd);
		}
	}
	clipper.End();
	ImGui::PopStyleVar();

	if (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
	{
		ImGui::SetScrollHereY(1.0f);
	}

	ImGui::EndChild();
	ImGui::End();
}

// -----------------------------------------------------------------------------
}
