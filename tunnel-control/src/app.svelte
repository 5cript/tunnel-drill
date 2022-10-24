<script>
	import Topbar from "./topbar.svelte";
	import Menu from "./menu.svelte";
	import Login from "./login.svelte";
	import PublisherSettings from "./publisher_settings.svelte";
	import { Router, Route } from "svelte-navigator";

	let isMenuOpen = true;
	const onMenuToggle = () => {
		isMenuOpen = !isMenuOpen;
	};
</script>

<main>
	<Router>
		<Route path="/login">
			<Login />
		</Route>
		<Route path="/*">
			<Topbar class="topbar" on:toggleMenu={onMenuToggle} />
			<Menu class="menu" isOpen={isMenuOpen} />
			<div class="content">
				<Route path="/stats/combined">"Stats Combined"</Route>
				<Route path="/stats/publisher">"Stats Publisher"</Route>
				<Route path="/settings/publisher"><PublisherSettings /></Route>
			</div>
		</Route>
	</Router>
</main>

<style>
	main {
		text-align: center;
		display: grid;
		grid-template-columns: auto 1fr;
		grid-template-rows: auto 1fr;
		gap: 0px 0px;
		grid-template-areas:
			"topbar topbar"
			"menu content";
		margin: 0;
		padding: 0;
		height: 100%;
		width: 100%;
		height: 100%;
		background-color: #303030;
	}

	:global(.topbar) {
		height: 100%;
		grid-area: topbar;
	}

	:global(.menu) {
		grid-area: menu;
	}

	.content {
		grid-area: content;
		background-color: var(--bs-body-bg);
		width: 100%;
		height: 100%;
	}
</style>
