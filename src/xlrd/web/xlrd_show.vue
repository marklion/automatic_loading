<template>
    <div class="industrial-panel xlrd-box">
        <div class="panel-title">下料满度</div>

        <div class="xlrd-grid">
            <section class="xlrd-card">
                <div class="xlrd-card-title">前溜槽下料满度</div>
                <div class="xlrd-digit">{{ head_offset }}</div>
            </section>

            <section class="xlrd-card">
                <div class="xlrd-card-title">后溜槽下料满度</div>
                <div class="xlrd-digit">{{ tail_offset }}</div>
            </section>
        </div>
    </div>
</template>

<script setup>
import { computed } from "vue";
import { useStatusInfo } from "@/stores/status_info";

const statusInfoStore = useStatusInfo();
const head_offset = computed(() => {
    return statusInfoStore.xlrd0.offset || 0;
});
const tail_offset = computed(() => {
    return statusInfoStore.xlrd1.offset || 0;
});

</script>

<style lang="scss" scoped>
@import "../../public/web/web_common/industrial_theme.css";

.xlrd-box {
    display: flex;
    flex-direction: column;
    gap: 12px;
}

.xlrd-grid {
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 8px;
}

.xlrd-card {
    border: 1px solid rgba(95, 169, 231, 0.35);
    background: rgba(8, 28, 49, 0.38);
    border-radius: 8px;
    padding: 10px;
    display: flex;
    flex-direction: column;
    gap: 8px;
}

.xlrd-card-title {
    color: #bfe3ff;
    font-size: 13px;
    font-weight: 700;
}

.xlrd-digit {
    width: fit-content;
    min-width: 92px;
    padding: 8px 12px 7px;
    border-radius: 8px;
    border: 1px solid rgba(120, 255, 178, 0.35);
    background:
        linear-gradient(180deg, rgba(2, 16, 14, 0.96), rgba(6, 29, 25, 0.9)),
        radial-gradient(circle at top, rgba(58, 255, 166, 0.18), transparent 55%);
    color: #7dffba;
    font-family: "Courier New", "Lucida Console", monospace;
    font-size: 22px;
    font-weight: 700;
    letter-spacing: 1px;
    line-height: 1;
    text-align: center;
    box-shadow:
        inset 0 0 0 1px rgba(136, 255, 193, 0.08),
        0 0 12px rgba(40, 255, 151, 0.22);
    text-shadow:
        0 0 4px rgba(125, 255, 186, 0.9),
        0 0 10px rgba(125, 255, 186, 0.55);
    border-top-color: rgba(163, 255, 214, 0.55);
}

@media (max-width: 960px) {
    .xlrd-grid {
        grid-template-columns: 1fr;
    }
}
</style>