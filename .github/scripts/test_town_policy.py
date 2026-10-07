"""Freeze selected policies in the shipped Town JSONC declarations.

These are document-level regression tests, not game/runtime tests: they do not
exercise the player resolver, map generation, artifact placement or building
application. Conditions use the valid expression subset present in these files;
unexpected syntax, operators and variables fail rather than being ignored.
"""
import copy
from pathlib import Path
import re
import unittest

from validate_json import load_jsonc


TOWN_DIR = Path(__file__).resolve().parents[2] / "lib/edit/towns"
TOWN_NAMES = (
    "01_Outpost_Full", "01_Outpost_Lite", "01_Outpost_OnlyAngband",
    "02_Telmora", "03_Morivant", "04_Angwil", "05_Zul", "06_Rlyeh_Obsoleted",
)
FULL_STARTS = [(33, 131), (34, 177), (56, 17), (43, 72), (19, 176),
               (22, 92), (16, 176), (47, 108), (44, 109), (44, 109)]


def state(**overrides):
    """Supply explicit known inputs; never invent a value for an unknown variable."""
    values = {f"QUEST{number}": "0" for number in range(1, 35)}
    values.update(CLASS="Warrior", RACE="Human", REALM1="Life",
                  IRONMAN_DOWNWARD="0", LEAVING_QUEST="0")
    values.update({name: str(value) for name, value in overrides.items()})
    return values


def matches(rule, values):
    """Check only the documented subset used by the bundled Town policies.

    This is deliberately stricter than evaluate_condition_expression(): require
    one complete boolean expression, reject NUL/trailing text and unknown inputs,
    require whitespace before nested opening brackets (the game consumes one
    separator after words/closing brackets, even if that separator is a '['),
    require arguments (two for comparisons), and accept only integer comparison
    operands rather than C++ atoi conversions. Variables must have explicit
    fixture values; this is not a replacement for the game's player resolver.
    """
    expression = rule.get("when")
    if expression is None:
        return True
    if not isinstance(expression, str) or not expression or "\0" in expression:
        raise ValueError("Unexpected condition expression")
    if re.search(r"\S\[", expression):
        raise ValueError("Opening bracket must be separated by whitespace")
    tokens = re.findall(r"\[|\]|[^\s\[\]]+", expression)
    position = 0

    def evaluate():
        nonlocal position
        if position >= len(tokens):
            raise ValueError("Incomplete condition expression")
        token = tokens[position]
        position += 1
        if token == "]":
            raise ValueError("Unexpected closing bracket")
        if token != "[":
            if not token.startswith("$"):
                return token
            name = token[1:]
            if name not in values:
                raise ValueError(f"Unsupported condition fixture variable: {token}; "
                                 "add an explicit fixture value for supported game variables")
            return values[name]
        if position >= len(tokens):
            raise ValueError("Missing condition operator")
        operator = tokens[position]
        position += 1
        arguments = []
        while position < len(tokens) and tokens[position] != "]":
            arguments.append(evaluate())
        if position >= len(tokens):
            raise ValueError("Unclosed condition")
        position += 1
        if operator not in ("AND", "IOR", "NOT", "EQU", "LEQ", "GEQ"):
            raise ValueError(f"Unsupported condition operator: {operator}")
        minimum = 2 if operator in ("EQU", "LEQ", "GEQ") else 1
        if len(arguments) < minimum:
            raise ValueError(f"Condition operator {operator} requires at least {minimum} arguments")
        # Within the supported subset, EQU
        # compares the first argument to ANY later argument; LEQ/GEQ compare
        # numeric values, not lexical strings; all arguments have been read.
        if operator == "AND":
            result = all(argument != "0" for argument in arguments)
        elif operator == "IOR":
            result = any(argument not in ("", "0") for argument in arguments)
        elif operator == "NOT":
            result = all(argument != "1" for argument in arguments)
        elif operator == "EQU":
            result = arguments[0] in arguments[1:]
        else:  # LEQ / GEQ
            if not all(re.fullmatch(r"-?[0-9]+", argument) for argument in arguments):
                raise ValueError("Unexpected non-integer comparison")
            first, *others = map(int, arguments)
            result = all(first <= other if operator == "LEQ" else first >= other for other in others)
        return "1" if result else "0"

    result = evaluate()
    if position != len(tokens) or result not in ("0", "1"):
        raise ValueError("Condition must be one complete boolean expression")
    return result != "0"


def selected_definitions(document, symbol, values):
    # Declaration order is significant: all matching rules are applied and the
    # last matching definition for a symbol replaces the previous definition.
    return [rule["definition"] for rule in document["featureRules"]
            if rule["symbol"] == symbol and matches(rule, values)]


class TownDeclarationPolicyTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Resolve from this source file, independently of the invocation cwd.
        cls.documents = {name: load_jsonc(TOWN_DIR / f"{name}.jsonc") for name in TOWN_NAMES}

    def assert_reward(self, document, symbol, values, expected):
        definitions = selected_definitions(document, symbol, values)
        self.assertTrue(definitions)
        reward = definitions[-1]
        self.assertEqual((reward["terrain"], reward["object"], reward["artifact"]),
                         ("FLOOR", *expected))

    def test_town_inventory_and_maps(self):
        self.assertEqual({path.stem for path in TOWN_DIR.glob("*.jsonc")}, set(TOWN_NAMES))
        for name, document in self.documents.items():
            with self.subTest(town=name):
                self.assertEqual(document["version"], 2)
                self.assertEqual(len(document["mapVariants"]), 2 if name == "06_Rlyeh_Obsoleted" else 1)
                height, width = (22, 66) if name == "01_Outpost_OnlyAngband" else (66, 198)
                for variant in document["mapVariants"]:
                    self.assertEqual(len(variant["rows"]), height)
                    self.assertEqual({len(row) for row in variant["rows"]}, {width})

    def test_every_declared_condition_is_supported(self):
        for name, document in self.documents.items():
            for collection in ("featureRules", "buildingRules", "mapVariants", "startingPositions"):
                for index, rule in enumerate(document[collection]):
                    with self.subTest(town=name, collection=collection, index=index):
                        self.assertIsInstance(matches(rule, state()), bool)
        for expression in ("[UNKNOWN 1]", "[EQU $UNKNOWN 0]", "[EQU 1 1", "[EQU 1 1] trailing"):
            with self.subTest(expression=expression), self.assertRaises(ValueError):
                matches({"when": expression}, state())

    def test_condition_subset_contract(self):
        cases = (
            ("[EQU 3 3 4]", True),
            ("[EQU 3 4 6]", False),
            ("[LEQ 10 2]", False),
            ("[GEQ 10 2]", True),
            ("[LEQ 2 10 1]", False),
            ("[AND [NOT [EQU 1 0]] [IOR 0 [EQU 2 2]]]", True),
            ("[NOT [IOR 0 [EQU 2 2]]]", False),
        )
        for expression, expected in cases:
            with self.subTest(expression=expression):
                self.assertEqual(matches({"when": expression}, state()), expected)
        # Production parses all arguments even after AND's result becomes false.
        with self.assertRaisesRegex(ValueError, r"fixture variable: \$UNKNOWN"):
            matches({"when": "[AND 0 [EQU $UNKNOWN 0]]"}, state())

    def test_condition_fixture_variable_diagnostics(self):
        # LEVEL is valid in the game, but no player level is assumed here.
        for variable in ("LEVEL", "QUEST35", "UNKNOWN"):
            with self.subTest(variable=variable), self.assertRaisesRegex(
                    ValueError, rf"fixture variable: \${variable}; add an explicit fixture value"):
                matches({"when": f"[EQU ${variable} 0]"}, state())
        self.assertTrue(matches({"when": "[EQU $LEVEL 10]"}, state(LEVEL=10)))

    def test_condition_subset_rejects_runtime_permissive_inputs(self):
        for operator in ("EQU", "LEQ", "GEQ"):
            for arguments in ("", " 1"):
                with self.subTest(operator=operator, arguments=arguments), self.assertRaisesRegex(
                        ValueError, f"operator {operator} requires at least 2 arguments"):
                    matches({"when": f"[{operator}{arguments}]"}, state())
        for expression in ("[AND]", "[IOR]", "[NOT]", "[LEQ abc 1]",
                           "[EQU a a] trailing", "[EQU a a]\0"):
            with self.subTest(expression=expression), self.assertRaises(ValueError):
                matches({"when": expression}, state())
        with self.assertRaisesRegex(ValueError, "Unsupported condition operator: UNKNOWN"):
            matches({"when": "[UNKNOWN 1]"}, state())
        for expression in ("[NOT[EQU a a]]", "[AND [EQU a a][EQU a b]]"):
            with self.subTest(expression=expression), self.assertRaisesRegex(
                    ValueError, "Opening bracket must be separated by whitespace"):
                matches({"when": expression}, state())

    def test_outpost_quest1_mode_specific_rewards(self):
        full = self.documents["01_Outpost_Full"]
        lite = self.documents["01_Outpost_Lite"]
        expected = {"Warrior": "42", "Elementalist": "126", "Mage": "618",
                    "High-Mage": "618", "Blue-Mage": "618", "Sorcerer": "618", "Mirror-Master": "618"}
        for class_name, lite_object in expected.items():
            with self.subTest(class_name=class_name):
                values = state(QUEST1=3, CLASS=class_name)
                self.assert_reward(full, "!", values, ("42", "0"))
                self.assert_reward(lite, "!", values, (lite_object, "0"))
        # The fallback precedes the class override, not just a set of rewards.
        definitions = selected_definitions(lite, "!", state(QUEST1=3, CLASS="Elementalist"))
        self.assertEqual([definition["object"] for definition in definitions][-2:], ["42", "126"])

    def test_outpost_quest27_mode_specific_rewards(self):
        for class_name, full_artifact, lite_artifact in (("Cavalry", "255", "163"),
                                                       ("Bard", "251", "15"),
                                                       ("Elementalist", "15", "80")):
            with self.subTest(class_name=class_name):
                values = state(QUEST27=3, CLASS=class_name)
                self.assert_reward(self.documents["01_Outpost_Full"], "@", values, ("0", full_artifact))
                self.assert_reward(self.documents["01_Outpost_Lite"], "!", values, ("0", lite_artifact))

    def test_outpost_progression_and_ironman_override(self):
        chains = {
            "01_Outpost_Full": ((1, 14), (14, 18), (18, 25), (25, 28), (28, 0)),
            "01_Outpost_Lite": ((1, 2), (2, 3), (3, 4), (4, 5), (5, 27), (27, 15), (15, 0)),
        }
        for name, chain in chains.items():
            for quest, next_quest in chain:
                for status in (1, 2, 3, 4, 5, 6):
                    # Full QUEST14's failed-state behavior is not an established
                    # progression policy, so do not freeze its baseline fallback.
                    if name == "01_Outpost_Full" and quest == 14 and status == 5:
                        continue
                    with self.subTest(town=name, quest=quest, status=status):
                        values = state(**{f"QUEST{quest}": status})
                        last = selected_definitions(self.documents[name], "b", values)[-1]
                        expected = quest if status in (1, 2, 5) else next_quest
                        self.assertEqual(last["special"], expected)
        for status in (3, 4, 6):
            definitions = selected_definitions(self.documents["01_Outpost_Full"], "b",
                                               state(QUEST1=status, IRONMAN_DOWNWARD=1))
            self.assertEqual([definition["special"] for definition in definitions][-2:], [14, 18])

    def test_telmora_death_paladin_insertion(self):
        town = self.documents["02_Telmora"]
        for class_name, realm, next_quest in (("Paladin", "Death", 16), ("Paladin", "Life", 26),
                                             ("Warrior", "Death", 26)):
            for status in (3, 4, 5, 6):
                with self.subTest(class_name=class_name, realm=realm, status=status):
                    definitions = selected_definitions(town, "b", state(QUEST5=status, CLASS=class_name, REALM1=realm))
                    self.assertEqual(definitions[-1]["special"], 5 if status == 5 else next_quest)
                    if next_quest == 16 and status != 5:
                        self.assertEqual([definition["special"] for definition in definitions][-2:], [26, 16])
        self.assertEqual(selected_definitions(town, "b", state(QUEST16=3, CLASS="Paladin", REALM1="Death"))[-1]["special"], 26)

    def test_morivant_cross_town_prerequisite(self):
        town = self.documents["03_Morivant"]
        for status24 in (3, 4, 6):
            for status19 in range(8):
                with self.subTest(status24=status24, status19=status19):
                    definition = selected_definitions(town, "b", state(QUEST24=status24, QUEST19=status19))[-1]
                    self.assertEqual(definition["special"], 21 if status19 >= 4 else 0)
        self.assertEqual(selected_definitions(town, "b", state(QUEST24=5, QUEST19=4))[-1]["special"], 24)

    def test_selected_and_ordered_starts(self):
        cases = (
            ("01_Outpost_Full", state(RACE="Vampire"), (33, 131)),
            ("01_Outpost_Lite", state(), (33, 131)),
            ("01_Outpost_Lite", state(RACE="Vampire"), (31, 150)),
            ("01_Outpost_Full", state(LEAVING_QUEST=27), (19, 176)),
            ("01_Outpost_Lite", state(LEAVING_QUEST=27), (19, 176)),
            ("02_Telmora", state(LEAVING_QUEST=16), (57, 157)),
            ("01_Outpost_OnlyAngband", state(), (10, 34)),
        )
        for name, values, expected in cases:
            with self.subTest(town=name, values=values):
                starts = [(rule["y"], rule["x"]) for rule in self.documents[name]["startingPositions"] if matches(rule, values)]
                self.assertEqual(starts, [expected])
        # Preserve the actual declaration sequence even where the conditions are
        # currently disjoint; runtime applies every matching start in this order.
        self.assertEqual([(rule["y"], rule["x"]) for rule in self.documents["01_Outpost_Full"]["startingPositions"]],
                         FULL_STARTS)

    def test_locales_have_identical_ordered_non_display_building_fields(self):
        for name, document in self.documents.items():
            with self.subTest(town=name):
                normalized = self.normalized_buildings(name, document)
                self.assertEqual(normalized["ja"], normalized["en"])

    def normalized_buildings(self, name, document):
        normalized = {"ja": [], "en": []}
        for index, rule in enumerate(document["buildingRules"]):
            fields = list(rule["fields"])
            if rule["command"] == "N":
                fields = []  # Building name, owner name and owner race text.
            elif rule["command"] == "A":
                self.assertGreaterEqual(len(fields), 2,
                                        f"{name}: buildingRules[{index}] command A needs an action name field")
                fields[1] = ""  # Display action name only; keep all numeric fields.
            normalized[rule["locale"]].append((rule["index"], rule["command"], rule.get("when"), fields))
        return normalized

    def test_short_building_action_fields_have_location(self):
        name = "01_Outpost_Lite"
        document = copy.deepcopy(self.documents[name])
        index = next(index for index, rule in enumerate(document["buildingRules"]) if rule["command"] == "A")
        for fields in ([], ["0"]):
            document["buildingRules"][index]["fields"] = fields
            with self.subTest(fields=fields), self.assertRaisesRegex(
                    AssertionError, rf"{name}: buildingRules\[{index}\] command A needs an action name field"):
                self.normalized_buildings(name, document)

    def test_real_data_override_mutations_are_detected(self):
        values = state(QUEST1=3, CLASS="Elementalist")
        lite = copy.deepcopy(self.documents["01_Outpost_Lite"])
        selected_definitions(lite, "!", values)[-1]["object"] = "42"
        with self.assertRaises(AssertionError):
            self.assert_reward(lite, "!", values, ("126", "0"))
        lite = copy.deepcopy(self.documents["01_Outpost_Lite"])
        lite["featureRules"].reverse()
        with self.assertRaises(AssertionError):
            self.assert_reward(lite, "!", values, ("126", "0"))
        full = copy.deepcopy(self.documents["01_Outpost_Full"])
        values = state(QUEST27=3, CLASS="Cavalry")
        selected_definitions(full, "@", values)[-1]["artifact"] = "163"
        with self.assertRaises(AssertionError):
            self.assert_reward(full, "@", values, ("0", "255"))
        full["startingPositions"].reverse()
        with self.assertRaises(AssertionError):
            self.assertEqual([(rule["y"], rule["x"]) for rule in full["startingPositions"]], FULL_STARTS)


if __name__ == "__main__":
    unittest.main()
