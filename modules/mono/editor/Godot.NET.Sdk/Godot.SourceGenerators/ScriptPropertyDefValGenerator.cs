using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using Microsoft.CodeAnalysis.Text;

namespace Godot.SourceGenerators
{
    [Generator]
    public class ScriptPropertyDefValGenerator : ISourceGenerator
    {
        public void Initialize(GeneratorInitializationContext context)
        {
        }

        public void Execute(GeneratorExecutionContext context)
        {
            if (context.AreGodotSourceGeneratorsDisabled())
                return;

            INamedTypeSymbol[] godotClasses = context
                .Compilation.SyntaxTrees
                .SelectMany(tree =>
                    tree.GetRoot().DescendantNodes()
                        .OfType<ClassDeclarationSyntax>()
                        .SelectGodotScriptClasses(context.Compilation)
                        // Report and skip non-partial classes
                        .Where(x =>
                        {
                            if (x.cds.IsPartial())
                            {
                                if (x.cds.IsNested() && !x.cds.AreAllOuterTypesPartial(out var typeMissingPartial))
                                {
                                    Common.ReportNonPartialGodotScriptOuterClass(context, typeMissingPartial!);
                                    return false;
                                }

                                return true;
                            }

                            Common.ReportNonPartialGodotScriptClass(context, x.cds, x.symbol);
                            return false;
                        })
                        .Select(x => x.symbol)
                )
                .Distinct<INamedTypeSymbol>(SymbolEqualityComparer.Default)
                .ToArray();

            if (godotClasses.Length > 0)
            {
                var typeCache = new MarshalUtils.TypeCache(context.Compilation);

                foreach (var godotClass in godotClasses)
                {
                    VisitGodotScriptClass(context, typeCache, godotClass);
                }
            }
        }

        private static void VisitGodotScriptClass(
            GeneratorExecutionContext context,
            MarshalUtils.TypeCache typeCache,
            INamedTypeSymbol symbol
        )
        {
            INamespaceSymbol namespaceSymbol = symbol.ContainingNamespace;
            string classNs = namespaceSymbol != null && !namespaceSymbol.IsGlobalNamespace ?
                namespaceSymbol.FullQualifiedName() :
                string.Empty;
            bool hasNamespace = classNs.Length != 0;

            bool isInnerClass = symbol.ContainingType != null;

            string uniqueHint = symbol.FullQualifiedName().SanitizeQualifiedNameForUniqueHint()
                                + "_ScriptPropertyDefVal_Generated";

            var source = new StringBuilder();

            source.Append("using Godot;\n");
            source.Append("using Godot.NativeInterop;\n");
            source.Append("\n");

            if (hasNamespace)
            {
                source.Append("namespace ");
                source.Append(classNs);
                source.Append(" {\n\n");
            }

            if (isInnerClass)
            {
                var containingType = symbol.ContainingType;

                while (containingType != null)
                {
                    source.Append("partial ");
                    source.Append(containingType.GetDeclarationKeyword());
                    source.Append(" ");
                    source.Append(containingType.NameWithTypeParameters());
                    source.Append("\n{\n");

                    containingType = containingType.ContainingType;
                }
            }

            source.Append("partial class ");
            source.Append(symbol.NameWithTypeParameters());
            source.Append("\n{\n");

            var exportedMembers = new List<ExportedPropertyMetadata>();

            var members = symbol.GetMembers();

            var exportedProperties = members
                .Where(s => !s.IsStatic && s.Kind == SymbolKind.Property)
                .Cast<IPropertySymbol>()
                .Where(s => s.GetAttributes()
                    .Any(a => a.AttributeClass?.IsGodotExportAttribute() ?? false))
                .ToArray();

            var exportedFields = members
                .Where(s => !s.IsStatic && s.Kind == SymbolKind.Field && !s.IsImplicitlyDeclared)
                .Cast<IFieldSymbol>()
                .Where(s => s.GetAttributes()
                    .Any(a => a.AttributeClass?.IsGodotExportAttribute() ?? false))
                .ToArray();

            foreach (var property in exportedProperties)
            {
                if (property.IsStatic)
                {
                    Common.ReportExportedMemberIsStatic(context, property);
                    continue;
                }

                if (property.IsIndexer)
                {
                    Common.ReportExportedMemberIsIndexer(context, property);
                    continue;
                }

                // TODO: We should still restore read-only properties after reloading assembly. Two possible ways: reflection or turn RestoreGodotObjectData into a constructor overload.
                // Ignore properties without a getter or without a setter. Godot properties must be both readable and writable.
                if (property.IsWriteOnly)
                {
                    Common.ReportExportedMemberIsWriteOnly(context, property);
                    continue;
                }

                if (property.IsReadOnly)
                {
                    Common.ReportExportedMemberIsReadOnly(context, property);
                    continue;
                }

                var propertyType = property.Type;
                var marshalType = MarshalUtils.ConvertManagedTypeToMarshalType(propertyType, typeCache);

                if (marshalType == null)
                {
                    Common.ReportExportedMemberTypeNotSupported(context, property);
                    continue;
                }

                // TODO: Detect default value from simple property getters (currently we only detect from initializers)

                EqualsValueClauseSyntax? initializer = property.DeclaringSyntaxReferences
                    .Select(r => r.GetSyntax() as PropertyDeclarationSyntax)
                    .Select(s => s?.Initializer ?? null)
                    .FirstOrDefault();

                string? value = initializer?.Value.ToString();

                exportedMembers.Add(new ExportedPropertyMetadata(
                    property.Name, marshalType.Value, propertyType, value));
            }

            foreach (var field in exportedFields)
            {
                if (field.IsStatic)
                {
                    Common.ReportExportedMemberIsStatic(context, field);
                    continue;
                }

                // TODO: We should still restore read-only fields after reloading assembly. Two possible ways: reflection or turn RestoreGodotObjectData into a constructor overload.
                // Ignore properties without a getter or without a setter. Godot properties must be both readable and writable.
                if (field.IsReadOnly)
                {
                    Common.ReportExportedMemberIsReadOnly(context, field);
                    continue;
                }

                var fieldType = field.Type;
                var marshalType = MarshalUtils.ConvertManagedTypeToMarshalType(fieldType, typeCache);

                if (marshalType == null)
                {
                    Common.ReportExportedMemberTypeNotSupported(context, field);
                    continue;
                }

                EqualsValueClauseSyntax? initializer = field.DeclaringSyntaxReferences
                    .Select(r => r.GetSyntax())
                    .OfType<VariableDeclaratorSyntax>()
                    .Select(s => s.Initializer)
                    .FirstOrDefault(i => i != null);

                string? value = initializer?.Value.ToString();

                exportedMembers.Add(new ExportedPropertyMetadata(
                    field.Name, marshalType.Value, fieldType, value));
            }

            // Generate GetGodotExportedProperties

            if (exportedMembers.Count > 0)
            {
                source.Append("#pragma warning disable CS0109 // Disable warning about redundant 'new' keyword\n");

                string dictionaryType = "System.Collections.Generic.Dictionary<StringName, object>";

                source.Append("#if TOOLS\n");
                source.Append("    internal new static ");
                source.Append(dictionaryType);
                source.Append(" GetGodotPropertyDefaultValues()\n    {\n");

                source.Append("        var values = new ");
                source.Append(dictionaryType);
                source.Append("(");
                source.Append(exportedMembers.Count);
                source.Append(");\n");

                foreach (var exportedMember in exportedMembers)
                {
                    string defaultValueLocalName = string.Concat("__", exportedMember.Name, "_default_value");

                    source.Append("        ");
                    source.Append(exportedMember.TypeSymbol.FullQualifiedName());
                    source.Append(" ");
                    source.Append(defaultValueLocalName);
                    source.Append(" = ");
                    source.Append(exportedMember.Value ?? "default");
                    source.Append(";\n");
                    source.Append("        values.Add(PropertyName.");
                    source.Append(exportedMember.Name);
                    source.Append(", ");
                    source.Append(defaultValueLocalName);
                    source.Append(");\n");
                }

                source.Append("        return values;\n");
                source.Append("    }\n");
                source.Append("#endif\n");

                source.Append("#pragma warning restore CS0109\n");
            }

            source.Append("}\n"); // partial class

            if (isInnerClass)
            {
                var containingType = symbol.ContainingType;

                while (containingType != null)
                {
                    source.Append("}\n"); // outer class

                    containingType = containingType.ContainingType;
                }
            }

            if (hasNamespace)
            {
                source.Append("\n}\n");
            }

            context.AddSource(uniqueHint, SourceText.From(source.ToString(), Encoding.UTF8));
        }

        private struct ExportedPropertyMetadata
        {
            public ExportedPropertyMetadata(string name, MarshalType type, ITypeSymbol typeSymbol, string? value)
            {
                Name = name;
                Type = type;
                TypeSymbol = typeSymbol;
                Value = value;
            }

            public string Name { get; }
            public MarshalType Type { get; }
            public ITypeSymbol TypeSymbol { get; }
            public string? Value { get; }
        }
    }
}
