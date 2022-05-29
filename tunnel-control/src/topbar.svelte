<script>
	import {
		Navbar,
		Button,
		Dropdown,
		DropdownToggle,
		DropdownMenu,
		DropdownItem,
	} from "sveltestrap";
	import { createEventDispatcher, onDestroy } from "svelte";
	import { useNavigate } from "svelte-navigator";
	import { isLoggedIn } from "./stores.js";
	const navigate = useNavigate();

	const dispatch = createEventDispatcher();
	let class_;
	export { class_ as class };
</script>

<div class={class_}>
	<Navbar class="py-0" expand="md">
		<Button
			dark
			on:click={() => {
				dispatch("toggleMenu");
			}}>Menu</Button
		>
		<h1 class="text-light">Tunnel Control</h1>
		<Dropdown>
			<DropdownToggle nav caret>User Control</DropdownToggle>
			<DropdownMenu end>
				{#if $isLoggedIn}
					<DropdownItem>Profile</DropdownItem>
					<DropdownItem>Publishers</DropdownItem>
					<DropdownItem divider />
					<DropdownItem
						on:click={() => {
							localStorage.removeItem("sessionToken");
							$isLoggedIn = false;
						}}>Sign Out</DropdownItem
					>
				{:else}
					<DropdownItem
						on:click={() => {
							navigate("/login");
						}}
					>
						Sign In
					</DropdownItem>
				{/if}
			</DropdownMenu>
		</Dropdown>
	</Navbar>
</div>

<style>
</style>
