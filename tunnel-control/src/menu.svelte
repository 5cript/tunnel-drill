<script>
	import {
		Collapse,
		Accordion,
		AccordionItem,
		ListGroup,
		ListGroupItem,
	} from "sveltestrap";
	import { Link } from "svelte-navigator";

	export let isOpen;
	let class_;
	export { class_ as class };
</script>

<Collapse
	class={`${class_} menu-internal`}
	{isOpen}
	horizontal
	data-bs-toggle="collapse"
>
	<Accordion stayOpen class="menu-accordion">
		<AccordionItem active header="Monitoring">
			<ListGroup flush class="menu-list-group">
				<ListGroupItem>
					<Link to="/stats/combined">Combined</Link>
				</ListGroupItem>
				<ListGroupItem>
					<Link to="/stats/publisher">By Publisher</Link>
				</ListGroupItem>
			</ListGroup>
		</AccordionItem>
		<AccordionItem header="Settings">
			<ListGroup flush class="menu-list-group">
				<ListGroupItem>
					<Link to="/settings/publisher">Publisher</Link>
				</ListGroupItem>
			</ListGroup>
		</AccordionItem>
	</Accordion>
</Collapse>

<style>
	:global(.menu-internal) {
		/* background-color: rgba(var(--bs-dark-rgb), var(--bs-bg-opacity)); */
		background-color: var(--bs-gray-dark);
		height: 100%;
	}

	:global(.menu-accordion
			.accordion-item
			.accordion-collapse
			.accordion-body) {
		padding: 0;
	}

	:global(.menu-list-group .list-group-item a) {
		text-decoration: none;
		color: var(--bs-light);
	}
</style>
