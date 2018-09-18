# threading model
[Platform Event Thread]					[main_thread]						[render_thread]
	[kick off main_thread]
										[kick off render_thread]
	[platform event loop]	--send-->		[main_thread event loop]
